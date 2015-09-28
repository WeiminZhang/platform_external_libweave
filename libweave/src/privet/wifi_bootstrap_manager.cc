// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "libweave/src/privet/wifi_bootstrap_manager.h"

#include <base/logging.h>
#include <base/memory/weak_ptr.h>
#include <weave/enum_to_string.h>
#include <weave/provider/network.h>
#include <weave/provider/task_runner.h>
#include <weave/provider/wifi.h>

#include "libweave/src/bind_lambda.h"
#include "libweave/src/privet/constants.h"
#include "libweave/src/config.h"

namespace weave {
namespace privet {

using provider::Network;

WifiBootstrapManager::WifiBootstrapManager(const std::string& test_privet_ssid,
                                           Config* config,
                                           provider::TaskRunner* task_runner,
                                           provider::Network* network,
                                           provider::Wifi* wifi,
                                           CloudDelegate* gcd)
    : config_{config},
      task_runner_{task_runner},
      network_{network},
      wifi_{wifi},
      ssid_generator_{gcd, this},
      test_privet_ssid_{test_privet_ssid} {
  CHECK(network_);
  CHECK(task_runner_);
  CHECK(wifi_);
}

void WifiBootstrapManager::Init() {
  UpdateConnectionState();
  network_->AddConnectionChangedCallback(
      base::Bind(&WifiBootstrapManager::OnConnectivityChange,
                 lifetime_weak_factory_.GetWeakPtr()));
  if (config_->GetSettings().last_configured_ssid.empty()) {
    StartBootstrapping();
  } else {
    StartMonitoring();
  }
}

void WifiBootstrapManager::StartBootstrapping() {
  if (network_->GetConnectionState() == Network::State::kConnected) {
    // If one of the devices we monitor for connectivity is online, we need not
    // start an AP.  For most devices, this is a situation which happens in
    // testing when we have an ethernet connection.  If you need to always
    // start an AP to bootstrap WiFi credentials, then add your WiFi interface
    // to the device whitelist.
    StartMonitoring();
    return;
  }

  UpdateState(State::kBootstrapping);
  if (!config_->GetSettings().last_configured_ssid.empty()) {
    // If we have been configured before, we'd like to periodically take down
    // our AP and find out if we can connect again.  Many kinds of failures are
    // transient, and having an AP up prohibits us from connecting as a client.
    task_runner_->PostDelayedTask(
        FROM_HERE, base::Bind(&WifiBootstrapManager::OnBootstrapTimeout,
                              tasks_weak_factory_.GetWeakPtr()),
        base::TimeDelta::FromMinutes(10));
  }
  // TODO(vitalybuka): Add SSID probing.
  privet_ssid_ = GenerateSsid();
  CHECK(!privet_ssid_.empty());
  wifi_->StartAccessPoint(privet_ssid_);
  LOG_IF(INFO, config_->GetSettings().ble_setup_enabled)
      << "BLE Bootstrap start: not implemented.";
}

void WifiBootstrapManager::EndBootstrapping() {
  LOG_IF(INFO, config_->GetSettings().ble_setup_enabled)
      << "BLE Bootstrap stop: not implemented.";
  wifi_->StopAccessPoint();
  privet_ssid_.clear();
}

void WifiBootstrapManager::StartConnecting(const std::string& ssid,
                                           const std::string& passphrase) {
  VLOG(1) << "WiFi is attempting to connect. (ssid=" << ssid
          << ", pass=" << passphrase << ").";
  UpdateState(State::kConnecting);
  task_runner_->PostDelayedTask(
      FROM_HERE, base::Bind(&WifiBootstrapManager::OnConnectError,
                            tasks_weak_factory_.GetWeakPtr(), nullptr),
      base::TimeDelta::FromMinutes(3));
  wifi_->Connect(ssid, passphrase,
                 base::Bind(&WifiBootstrapManager::OnConnectSuccess,
                            tasks_weak_factory_.GetWeakPtr(), ssid),
                 base::Bind(&WifiBootstrapManager::OnConnectError,
                            tasks_weak_factory_.GetWeakPtr()));
}

void WifiBootstrapManager::OnConnectError(const Error* error) {
  ErrorPtr new_error = error ? error->Clone() : nullptr;
  Error::AddTo(&new_error, FROM_HERE, errors::kDomain, errors::kInvalidState,
               "Failed to connect to provided network");
  setup_state_ = SetupState{std::move(new_error)};
  StartBootstrapping();
}

void WifiBootstrapManager::EndConnecting() {}

void WifiBootstrapManager::StartMonitoring() {
  VLOG(1) << "Monitoring connectivity.";
  // We already have a callback in place with |network_| to update our
  // connectivity state.  See OnConnectivityChange().
  UpdateState(State::kMonitoring);

  if (network_->GetConnectionState() == Network::State::kConnected) {
    monitor_until_ = {};
  } else {
    if (monitor_until_.is_null()) {
      monitor_until_ = base::Time::Now() + base::TimeDelta::FromMinutes(2);
      VLOG(2) << "Waiting for connection until: " << monitor_until_;
    }

    // Schedule timeout timer taking into account already offline time.
    task_runner_->PostDelayedTask(
        FROM_HERE, base::Bind(&WifiBootstrapManager::OnMonitorTimeout,
                              tasks_weak_factory_.GetWeakPtr()),
        monitor_until_ - base::Time::Now());
  }
}

void WifiBootstrapManager::EndMonitoring() {}

void WifiBootstrapManager::UpdateState(State new_state) {
  VLOG(3) << "Switching state from " << static_cast<int>(state_) << " to "
          << static_cast<int>(new_state);
  // Abort irrelevant tasks.
  tasks_weak_factory_.InvalidateWeakPtrs();

  switch (state_) {
    case State::kDisabled:
      break;
    case State::kBootstrapping:
      EndBootstrapping();
      break;
    case State::kMonitoring:
      EndMonitoring();
      break;
    case State::kConnecting:
      EndConnecting();
      break;
  }

  state_ = new_state;
}

std::string WifiBootstrapManager::GenerateSsid() const {
  return test_privet_ssid_.empty() ? ssid_generator_.GenerateSsid()
                                   : test_privet_ssid_;
}

const ConnectionState& WifiBootstrapManager::GetConnectionState() const {
  return connection_state_;
}

const SetupState& WifiBootstrapManager::GetSetupState() const {
  return setup_state_;
}

bool WifiBootstrapManager::ConfigureCredentials(const std::string& ssid,
                                                const std::string& passphrase,
                                                ErrorPtr* error) {
  setup_state_ = SetupState{SetupState::kInProgress};
  // Since we are changing network, we need to let the web server send out the
  // response to the HTTP request leading to this action. So, we are waiting
  // a bit before mocking with network set up.
  task_runner_->PostDelayedTask(
      FROM_HERE, base::Bind(&WifiBootstrapManager::StartConnecting,
                            tasks_weak_factory_.GetWeakPtr(), ssid, passphrase),
      base::TimeDelta::FromSeconds(1));
  return true;
}

std::string WifiBootstrapManager::GetCurrentlyConnectedSsid() const {
  // TODO(vitalybuka): Get from shill, if possible.
  return config_->GetSettings().last_configured_ssid;
}

std::string WifiBootstrapManager::GetHostedSsid() const {
  return privet_ssid_;
}

std::set<WifiType> WifiBootstrapManager::GetTypes() const {
  // TODO(wiley) This should do some system work to figure this out.
  return {WifiType::kWifi24};
}

void WifiBootstrapManager::OnConnectSuccess(const std::string& ssid) {
  VLOG(1) << "Wifi was connected successfully";
  Config::Transaction change{config_};
  change.set_last_configured_ssid(ssid);
  change.Commit();
  setup_state_ = SetupState{SetupState::kSuccess};
  StartMonitoring();
}

void WifiBootstrapManager::OnBootstrapTimeout() {
  VLOG(1) << "Bootstrapping has timed out.";
  StartMonitoring();
}

void WifiBootstrapManager::OnConnectivityChange() {
  VLOG(3) << "ConnectivityChanged: "
          << EnumToString(network_->GetConnectionState());
  UpdateConnectionState();

  if (state_ == State::kMonitoring ||  // Reset monitoring timeout.
      (state_ != State::kDisabled &&
       network_->GetConnectionState() == Network::State::kConnected)) {
    StartMonitoring();
  }
}

void WifiBootstrapManager::OnMonitorTimeout() {
  VLOG(1) << "Spent too long offline.  Entering bootstrap mode.";
  // TODO(wiley) Retrieve relevant errors from shill.
  StartBootstrapping();
}

void WifiBootstrapManager::UpdateConnectionState() {
  connection_state_ = ConnectionState{ConnectionState::kUnconfigured};
  if (config_->GetSettings().last_configured_ssid.empty())
    return;

  Network::State service_state{network_->GetConnectionState()};
  switch (service_state) {
    case Network::State::kOffline:
      connection_state_ = ConnectionState{ConnectionState::kOffline};
      return;
    case Network::State::kFailure: {
      // TODO(wiley) Pull error information from somewhere.
      ErrorPtr error;
      Error::AddTo(&error, FROM_HERE, errors::kDomain, errors::kInvalidState,
                   "Unknown WiFi error");
      connection_state_ = ConnectionState{std::move(error)};
      return;
    }
    case Network::State::kConnecting:
      connection_state_ = ConnectionState{ConnectionState::kConnecting};
      return;
    case Network::State::kConnected:
      connection_state_ = ConnectionState{ConnectionState::kOnline};
      return;
  }
  ErrorPtr error;
  Error::AddToPrintf(&error, FROM_HERE, errors::kDomain, errors::kInvalidState,
                     "Unknown network state: %s",
                     EnumToString(service_state).c_str());
  connection_state_ = ConnectionState{std::move(error)};
}

}  // namespace privet
}  // namespace weave
