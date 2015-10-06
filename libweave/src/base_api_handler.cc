// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/base_api_handler.h"

#include <base/bind.h>
#include <weave/device.h>

#include "src/device_registration_info.h"

namespace weave {

namespace {
const char kBaseStateFirmwareVersion[] = "base.firmwareVersion";
const char kBaseStateAnonymousAccessRole[] = "base.localAnonymousAccessMaxRole";
const char kBaseStateDiscoveryEnabled[] = "base.localDiscoveryEnabled";
const char kBaseStatePairingEnabled[] = "base.localPairingEnabled";
}  // namespace

BaseApiHandler::BaseApiHandler(DeviceRegistrationInfo* device_info,
                               Device* device)
    : device_info_{device_info}, device_{device} {
  device_info_->GetMutableConfig()->AddOnChangedCallback(base::Bind(
      &BaseApiHandler::OnConfigChanged, weak_ptr_factory_.GetWeakPtr()));

  const auto& settings = device_info_->GetSettings();
  base::DictionaryValue state;
  state.SetStringWithoutPathExpansion(kBaseStateFirmwareVersion,
                                      settings.firmware_version);
  CHECK(device_->SetStateProperties(state, nullptr));

  device_->AddCommandHandler(
      "base.updateBaseConfiguration",
      base::Bind(&BaseApiHandler::UpdateBaseConfiguration,
                 weak_ptr_factory_.GetWeakPtr()));

  device_->AddCommandHandler("base.updateDeviceInfo",
                             base::Bind(&BaseApiHandler::UpdateDeviceInfo,
                                        weak_ptr_factory_.GetWeakPtr()));
}

void BaseApiHandler::UpdateBaseConfiguration(Command* command) {
  CHECK(command->GetStatus() == CommandStatus::kQueued)
      << EnumToString(command->GetStatus());
  command->SetProgress(base::DictionaryValue{}, nullptr);

  const auto& settings = device_info_->GetSettings();
  std::string anonymous_access_role{
      EnumToString(settings.local_anonymous_access_role)};
  bool discovery_enabled{settings.local_discovery_enabled};
  bool pairing_enabled{settings.local_pairing_enabled};

  auto parameters = command->GetParameters();
  parameters->GetString("localAnonymousAccessMaxRole", &anonymous_access_role);
  parameters->GetBoolean("localDiscoveryEnabled", &discovery_enabled);
  parameters->GetBoolean("localPairingEnabled", &pairing_enabled);

  AuthScope auth_scope{AuthScope::kNone};
  if (!StringToEnum(anonymous_access_role, &auth_scope)) {
    return command->Abort();
  }

  device_info_->UpdateBaseConfig(auth_scope, discovery_enabled,
                                 pairing_enabled);

  command->Done();
}

void BaseApiHandler::OnConfigChanged(const Settings& settings) {
  base::DictionaryValue state;
  state.SetStringWithoutPathExpansion(
      kBaseStateAnonymousAccessRole,
      EnumToString(settings.local_anonymous_access_role));
  state.SetBooleanWithoutPathExpansion(kBaseStateDiscoveryEnabled,
                                       settings.local_discovery_enabled);
  state.SetBooleanWithoutPathExpansion(kBaseStatePairingEnabled,
                                       settings.local_pairing_enabled);
  device_->SetStateProperties(state, nullptr);
}

void BaseApiHandler::UpdateDeviceInfo(Command* command) {
  CHECK(command->GetStatus() == CommandStatus::kQueued)
      << EnumToString(command->GetStatus());
  command->SetProgress(base::DictionaryValue{}, nullptr);

  const auto& settings = device_info_->GetSettings();
  std::string name{settings.name};
  std::string description{settings.description};
  std::string location{settings.location};

  auto parameters = command->GetParameters();
  parameters->GetString("name", &name);
  parameters->GetString("description", &description);
  parameters->GetString("location", &location);

  device_info_->UpdateDeviceInfo(name, description, location);
  command->Done();
}

}  // namespace weave
