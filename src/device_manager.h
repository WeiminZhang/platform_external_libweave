// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_DEVICE_MANAGER_H_
#define LIBWEAVE_SRC_DEVICE_MANAGER_H_

#include <base/memory/weak_ptr.h>
#include <weave/device.h>

namespace weave {

class AccessApiHandler;
class AccessRevocationManager;
class BaseApiHandler;
class Config;
class ComponentManager;
class DeviceRegistrationInfo;

namespace privet {
class AuthManager;
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
  const Settings& GetSettings() const override;
  void AddSettingsChangedCallback(
      const SettingsChangedCallback& callback) override;
  void AddTraitDefinitionsFromJson(const std::string& json) override;
  void AddTraitDefinitions(const base::DictionaryValue& dict) override;
  const base::DictionaryValue& GetTraits() const override;
  void AddTraitDefsChangedCallback(const base::Closure& callback) override;
  bool AddComponent(const std::string& name,
                    const std::vector<std::string>& traits,
                    ErrorPtr* error) override;
  bool RemoveComponent(const std::string& name, ErrorPtr* error) override;
  void AddComponentTreeChangedCallback(const base::Closure& callback) override;
  const base::DictionaryValue& GetComponents() const override;
  bool SetStatePropertiesFromJson(const std::string& component,
                                  const std::string& json,
                                  ErrorPtr* error) override;
  bool SetStateProperties(const std::string& component,
                          const base::DictionaryValue& dict,
                          ErrorPtr* error) override;
  const base::Value* GetStateProperty(const std::string& component,
                                      const std::string& name,
                                      ErrorPtr* error) const override;
  bool SetStateProperty(const std::string& component,
                        const std::string& name,
                        const base::Value& value,
                        ErrorPtr* error) override;
  void AddCommandHandler(const std::string& component,
                         const std::string& command_name,
                         const CommandHandlerCallback& callback) override;
  bool AddCommand(const base::DictionaryValue& command,
                  std::string* id,
                  ErrorPtr* error) override;
  Command* FindCommand(const std::string& id) override;
  void AddStateChangedCallback(const base::Closure& callback) override;
  void Register(const RegistrationData& registration_data,
                const DoneCallback& callback) override;
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

  std::unique_ptr<Config> config_;
  std::unique_ptr<privet::AuthManager> auth_manager_;
  std::unique_ptr<ComponentManager> component_manager_;
  std::unique_ptr<DeviceRegistrationInfo> device_info_;
  std::unique_ptr<BaseApiHandler> base_api_handler_;
  std::unique_ptr<AccessRevocationManager> access_revocation_manager_;
  std::unique_ptr<AccessApiHandler> access_api_handler_;
  std::unique_ptr<privet::Manager> privet_;

  base::WeakPtrFactory<DeviceManager> weak_ptr_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(DeviceManager);
};

}  // namespace weave

#endif  // LIBWEAVE_SRC_DEVICE_MANAGER_H_
