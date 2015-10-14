// Copyright 2015 The Weave Authors. All rights reserved.
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

DeviceManager::DeviceManager(provider::ConfigStore* config_store,
                             provider::TaskRunner* task_runner,
                             provider::HttpClient* http_client,
                             provider::Network* network,
                             provider::DnsServiceDiscovery* dns_sd,
                             provider::HttpServer* http_server,
                             provider::Wifi* wifi,
                             provider::Bluetooth* bluetooth) {
  command_manager_ = std::make_shared<CommandManager>();
  command_manager_->Startup();
  state_change_queue_.reset(new StateChangeQueue(kMaxStateChangeQueueSize));
  state_manager_ = std::make_shared<StateManager>(state_change_queue_.get());
  state_manager_->Startup();

  std::unique_ptr<Config> config{new Config{config_store}};
  config->Load();

  device_info_.reset(new DeviceRegistrationInfo(
      command_manager_, state_manager_, std::move(config), task_runner,
      http_client, network));
  base_api_handler_.reset(new BaseApiHandler{device_info_.get(), this});

  device_info_->Start();

  if (http_server) {
    StartPrivet(task_runner, network, dns_sd, http_server, wifi, bluetooth);
  } else {
    CHECK(!dns_sd);
  }
}

DeviceManager::~DeviceManager() {}

const Settings& DeviceManager::GetSettings() const {
  return device_info_->GetSettings();
}

void DeviceManager::AddSettingsChangedCallback(
    const SettingsChangedCallback& callback) {
  device_info_->GetMutableConfig()->AddOnChangedCallback(callback);
}

Config* DeviceManager::GetConfig() {
  return device_info_->GetMutableConfig();
}

void DeviceManager::StartPrivet(provider::TaskRunner* task_runner,
                                provider::Network* network,
                                provider::DnsServiceDiscovery* dns_sd,
                                provider::HttpServer* http_server,
                                provider::Wifi* wifi,
                                provider::Bluetooth* bluetooth) {
  privet_.reset(new privet::Manager{task_runner});
  privet_->Start(network, dns_sd, http_server, wifi, device_info_.get(),
                 command_manager_.get(), state_manager_.get());
}

GcdState DeviceManager::GetGcdState() const {
  return device_info_->GetGcdState();
}

void DeviceManager::AddGcdStateChangedCallback(
    const GcdStateChangedCallback& callback) {
  device_info_->AddGcdStateChangedCallback(callback);
}

void DeviceManager::AddCommandDefinitionsFromJson(const std::string& json) {
  CHECK(command_manager_->LoadCommands(json, nullptr));
}

void DeviceManager::AddCommandDefinitions(const base::DictionaryValue& dict) {
  CHECK(command_manager_->LoadCommands(dict, nullptr));
}

bool DeviceManager::AddCommand(const base::DictionaryValue& command,
                               std::string* id,
                               ErrorPtr* error) {
  return command_manager_->AddCommand(command, id, error);
}

Command* DeviceManager::FindCommand(const std::string& id) {
  return command_manager_->FindCommand(id);
}

void DeviceManager::AddCommandHandler(const std::string& command_name,
                                      const CommandHandlerCallback& callback) {
  return command_manager_->AddCommandHandler(command_name, callback);
}

void DeviceManager::AddStateChangedCallback(const base::Closure& callback) {
  state_manager_->AddChangedCallback(callback);
}

void DeviceManager::AddStateDefinitionsFromJson(const std::string& json) {
  CHECK(state_manager_->LoadStateDefinitionFromJson(json, nullptr));
}

void DeviceManager::AddStateDefinitions(const base::DictionaryValue& dict) {
  CHECK(state_manager_->LoadStateDefinition(dict, nullptr));
}

bool DeviceManager::SetStatePropertiesFromJson(const std::string& json,
                                               ErrorPtr* error) {
  return state_manager_->SetPropertiesFromJson(json, error);
}

bool DeviceManager::SetStateProperties(const base::DictionaryValue& dict,
                                       ErrorPtr* error) {
  return state_manager_->SetProperties(dict, error);
}

std::unique_ptr<base::Value> DeviceManager::GetStateProperty(
    const std::string& name) const {
  return state_manager_->GetProperty(name);
}

bool DeviceManager::SetStateProperty(const std::string& name,
                                     const base::Value& value,
                                     ErrorPtr* error) {
  return state_manager_->SetProperty(name, value, error);
}

std::unique_ptr<base::DictionaryValue> DeviceManager::GetState() const {
  return state_manager_->GetState();
}

void DeviceManager::Register(const std::string& ticket_id,
                             const DoneCallback& callback) {
  device_info_->RegisterDevice(ticket_id, callback);
}

void DeviceManager::AddPairingChangedCallbacks(
    const PairingBeginCallback& begin_callback,
    const PairingEndCallback& end_callback) {
  if (privet_)
    privet_->AddOnPairingChangedCallbacks(begin_callback, end_callback);
}

std::unique_ptr<Device> Device::Create(provider::ConfigStore* config_store,
                                       provider::TaskRunner* task_runner,
                                       provider::HttpClient* http_client,
                                       provider::Network* network,
                                       provider::DnsServiceDiscovery* dns_sd,
                                       provider::HttpServer* http_server,
                                       provider::Wifi* wifi,
                                       provider::Bluetooth* bluetooth) {
  return std::unique_ptr<Device>{
      new DeviceManager{config_store, task_runner, http_client, network, dns_sd,
                        http_server, wifi, bluetooth}};
}

}  // namespace weave
