// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_DEVICE_MANAGER_H_
#define LIBWEAVE_SRC_DEVICE_MANAGER_H_

#include <base/memory/weak_ptr.h>
#include <weave/device.h>

namespace weave {

class BaseApiHandler;
class Config;
class CommandManager;
class DeviceRegistrationInfo;
class StateChangeQueue;
class StateManager;

namespace privet {
class Manager;
}  // namespace privet

class DeviceManager final : public Device {
 public:
  DeviceManager(provider::ConfigStore* config_store,
                provider::TaskRunner* task_runner,
                provider::HttpClient* http_client,
                provider::Network* network,
                provider::DnsServiceDiscovery* dns_sd,
                provider::HttpServer* http_server,
                provider::Wifi* wifi,
                provider::Bluetooth* bluetooth);
  ~DeviceManager() override;

  // Device implementation.
  const Settings& GetSettings() override;
  void AddSettingsChangedCallback(
      const SettingsChangedCallback& callback) override;
  Commands* GetCommands() override;

  void AddStateChangedCallback(const base::Closure& callback) override;
  bool SetStateProperties(const base::DictionaryValue& property_set,
                          ErrorPtr* error) override;
  std::unique_ptr<base::Value> GetStateProperty(
      const std::string& name) const override;
  bool SetStateProperty(const std::string& name,
                        const base::Value& value,
                        ErrorPtr* error) override;
  std::unique_ptr<base::DictionaryValue> GetState() const override;

  std::string Register(const std::string& ticket_id, ErrorPtr* error) override;

  GcdState GetGcdState() const override;
  void AddGcdStateChangedCallback(
      const GcdStateChangedCallback& callback) override;

  void AddPairingChangedCallbacks(
      const PairingBeginCallback& begin_callback,
      const PairingEndCallback& end_callback) override;

  Config* GetConfig();

 private:
  void StartPrivet(provider::TaskRunner* task_runner,
                   provider::Network* network,
                   provider::DnsServiceDiscovery* dns_sd,
                   provider::HttpServer* http_server,
                   provider::Wifi* wifi,
                   provider::Bluetooth* bluetooth);

  std::shared_ptr<CommandManager> command_manager_;
  std::unique_ptr<StateChangeQueue> state_change_queue_;
  std::shared_ptr<StateManager> state_manager_;
  std::unique_ptr<DeviceRegistrationInfo> device_info_;
  std::unique_ptr<BaseApiHandler> base_api_handler_;
  std::unique_ptr<privet::Manager> privet_;

  base::WeakPtrFactory<DeviceManager> weak_ptr_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(DeviceManager);
};

}  // namespace weave

#endif  // LIBWEAVE_SRC_DEVICE_MANAGER_H_
