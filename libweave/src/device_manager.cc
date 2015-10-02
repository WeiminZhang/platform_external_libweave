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

DeviceManager::DeviceManager(provider::ConfigStore* config_store,
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

DeviceManager::~DeviceManager() {}

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

GcdState DeviceManager::GetGcdState() const {
  return device_info_->GetGcdState();
}

void DeviceManager::AddGcdStateChangedCallback(
    const GcdStateChangedCallback& callback) {
  device_info_->AddGcdStateChangedCallback(callback);
}

std::string DeviceManager::Register(const std::string& ticket_id,
                                    ErrorPtr* error) {
  return device_info_->RegisterDevice(ticket_id, error);
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
