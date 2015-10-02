// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_DEVICE_H_
#define LIBWEAVE_INCLUDE_WEAVE_DEVICE_H_

#include <memory>
#include <set>
#include <string>

#include <weave/commands.h>
#include <weave/export.h>
#include <weave/provider/bluetooth.h>
#include <weave/provider/config_store.h>
#include <weave/provider/dns_service_discovery.h>
#include <weave/provider/http_client.h>
#include <weave/provider/http_server.h>
#include <weave/provider/network.h>
#include <weave/provider/task_runner.h>
#include <weave/provider/wifi.h>
#include <weave/state.h>

namespace weave {

// States of Gcd connection.
enum class GcdState {
  kUnconfigured,        // We have no credentials.
  kConnecting,          // We have credentials but not yet connected.
  kConnected,           // We're registered and connected to the cloud.
  kInvalidCredentials,  // Our registration has been revoked.
};

class Device {
 public:
  virtual ~Device() = default;

  // Returns reference the current settings.
  virtual const Settings& GetSettings() = 0;

  // Callback type for AddSettingsChangedCallback.
  using SettingsChangedCallback =
      base::Callback<void(const Settings& settings)>;
  // Subscribes to notification settings changes.
  virtual void AddSettingsChangedCallback(
      const SettingsChangedCallback& callback) = 0;

  virtual Commands* GetCommands() = 0;
  virtual State* GetState() = 0;

  // Returns current state of GCD connection.
  virtual GcdState GetGcdState() const = 0;

  // Callback type for GcdStatusCallback.
  using GcdStateChangedCallback = base::Callback<void(GcdState state)>;
  // Sets callback which is called when state of server connection changed.
  virtual void AddGcdStateChangedCallback(
      const GcdStateChangedCallback& callback) = 0;

  // Registers the device. Returns a device ID on success.
  // This is testing method and should not be used by applications.
  virtual std::string Register(const std::string& ticket_id,
                               ErrorPtr* error) = 0;

  // Handler should display pin code to the user.
  using PairingBeginCallback =
      base::Callback<void(const std::string& session_id,
                          PairingType pairing_type,
                          const std::vector<uint8_t>& code)>;
  // Handler should stop displaying pin code.
  using PairingEndCallback =
      base::Callback<void(const std::string& session_id)>;
  // Subscribes to notification about client pairing events.
  virtual void AddPairingChangedCallbacks(
      const PairingBeginCallback& begin_callback,
      const PairingEndCallback& end_callback) = 0;

  LIBWEAVE_EXPORT static std::unique_ptr<Device> Create(
      provider::ConfigStore* config_store,
      provider::TaskRunner* task_runner,
      provider::HttpClient* http_client,
      provider::Network* network,
      provider::DnsServiceDiscovery* dns_sd,
      provider::HttpServer* http_server,
      provider::Wifi* wifi,
      provider::Bluetooth* bluetooth_provider);
};

}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_DEVICE_H_
