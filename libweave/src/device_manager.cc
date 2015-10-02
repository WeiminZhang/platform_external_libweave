// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/device_manager.h"

#include <string>

#include <base/bind.h>

#include "src/base_api_handler.h"
#include "src/commands/command_manager.h"
#include "src/config.h"
#include "src/device_registration_info.h"
#include "src/privet/privet_manager.h"
#include "src/states/state_change_queue.h"
#include "src/states/state_manager.h"

namespace weave {

namespace {

// Max of 100 state update events should be enough in the queue.
const size_t kMaxStateChangeQueueSize = 100;

}  // namespace

DeviceManager::DeviceManager() {}

DeviceManager::~DeviceManager() {}

void DeviceManager::Start(provider::ConfigStore* config_store,
                          provider::TaskRunner* task_runner,
                          provider::HttpClient* http_client,
                          provider::Network* network,
                          provider::DnsServiceDiscovery* dns_sd,
                          provider::HttpServer* http_server,
                          provider::Wifi* wifi,
                          provider::Bluetooth* bluetooth) {
  command_manager_ = std::make_shared<CommandManager>();
  command_manager_->Startup(config_store);
  state_change_queue_.reset(new StateChangeQueue(kMaxStateChangeQueueSize));
  state_manager_ = std::make_shared<StateManager>(state_change_queue_.get());
  state_manager_->Startup(config_store);

  std::unique_ptr<Config> config{new Config{config_store}};
  config->Load();

  // TODO(avakulenko): Figure out security implications of storing
  // device info state data unencrypted.
  device_info_.reset(new DeviceRegistrationInfo(
      command_manager_, state_manager_, std::move(config), task_runner,
      http_client, network));
  base_api_handler_.reset(
      new BaseApiHandler{device_info_.get(), state_manager_, command_manager_});

  device_info_->Start();

  if (http_server) {
    StartPrivet(task_runner, network, dns_sd, http_server, wifi, bluetooth);
  } else {
    CHECK(!dns_sd);
  }
}

const Settings& DeviceManager::GetSettings() {
  return device_info_->GetSettings();
}

void DeviceManager::AddSettingsChangedCallback(
    const SettingsChangedCallback& callback) {
  device_info_->GetMutableConfig()->AddOnChangedCallback(callback);
}

Commands* DeviceManager::GetCommands() {
  return command_manager_.get();
}

State* DeviceManager::GetState() {
  return state_manager_.get();
}

Config* DeviceManager::GetConfig() {
  return device_info_->GetMutableConfig();
}

Cloud* DeviceManager::GetCloud() {
  return device_info_.get();
}

void DeviceManager::StartPrivet(provider::TaskRunner* task_runner,
                                provider::Network* network,
                                provider::DnsServiceDiscovery* dns_sd,
                                provider::HttpServer* http_server,
                                provider::Wifi* wifi,
                                provider::Bluetooth* bluetooth) {
  privet_.reset(new privet::Manager{});
  privet_->Start(task_runner, network, dns_sd, http_server, wifi,
                 device_info_.get(), command_manager_.get(),
                 state_manager_.get());
}

void DeviceManager::AddPairingChangedCallbacks(
      const PairingBeginCallback& begin_callback,
      const PairingEndCallback& end_callback) {
  privet_->AddOnPairingChangedCallbacks(begin_callback, end_callback);
}

std::unique_ptr<Device> Device::Create() {
  return std::unique_ptr<Device>{new DeviceManager};
}

}  // namespace weave
