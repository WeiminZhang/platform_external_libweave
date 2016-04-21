// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/base_api_handler.h"

#include <base/bind.h>
#include <weave/device.h>

#include "src/commands/schema_constants.h"
#include "src/device_registration_info.h"

namespace weave {

namespace {
const char kDeviceComponent[] = "device";
const char kDeviceTrait[] = "device";
const char kPrivetTrait[] = "privet";
}  // namespace

BaseApiHandler::BaseApiHandler(DeviceRegistrationInfo* device_info,
                               Device* device)
    : device_info_{device_info}, device_{device} {
  device_->AddTraitDefinitionsFromJson(R"({
    "device": {
      "commands": {
        "setConfig": {
          "minimalRole": "user",
          "parameters": {
            "name": {
              "type": "string"
            },
            "description": {
              "type": "string"
            },
            "location": {
              "type": "string"
            }
          }
        }
      },
      "state": {
        "name": {
          "isRequired": true,
          "type": "string"
        },
        "description": {
          "isRequired": true,
          "type": "string"
        },
        "location": {
          "type": "string"
        },
        "hardwareId": {
          "isRequired": true,
          "type": "string"
        },
        "serialNumber": {
          "isRequired": true,
          "type": "string"
        },
        "firmwareVersion": {
          "isRequired": true,
          "type": "string"
        }
      }
    },
    "privet": {
      "commands": {
        "setConfig": {
          "minimalRole": "manager",
          "parameters": {
            "isLocalAccessEnabled": {
              "type": "boolean"
            },
            "maxRoleForAnonymousAccess": {
              "type": "string",
              "enum": [ "none", "viewer", "user", "manager" ]
            }
          }
        }
      },
      "state": {
        "apiVersion": {
          "isRequired": true,
          "type": "string"
        },
        "isLocalAccessEnabled": {
          "isRequired": true,
          "type": "boolean"
        },
        "maxRoleForAnonymousAccess": {
          "isRequired": true,
          "type": "string",
          "enum": [ "none", "viewer", "user", "manager" ]
        }
      }
    }
  })");
  CHECK(device_->AddComponent(kDeviceComponent, {kDeviceTrait, kPrivetTrait},
                              nullptr));
  OnConfigChanged(device_->GetSettings());

  const auto& settings = device_info_->GetSettings();
  base::DictionaryValue state;
  state.SetString("device.firmwareVersion", settings.firmware_version);
  state.SetString("device.hardwareId", settings.device_id);
  state.SetString("device.serialNumber", settings.serial_number);
  state.SetString("privet.apiVersion", "3");  // Presently Privet v3.
  CHECK(device_->SetStateProperties(kDeviceComponent, state, nullptr));

  device_->AddCommandHandler(
      kDeviceComponent, "device.setConfig",
      base::Bind(&BaseApiHandler::DeviceSetConfig,
                 weak_ptr_factory_.GetWeakPtr()));

  device_->AddCommandHandler(kDeviceComponent, "privet.setConfig",
                             base::Bind(&BaseApiHandler::PrivetSetConfig,
                                        weak_ptr_factory_.GetWeakPtr()));

  device_info_->GetMutableConfig()->AddOnChangedCallback(base::Bind(
      &BaseApiHandler::OnConfigChanged, weak_ptr_factory_.GetWeakPtr()));
}

void BaseApiHandler::PrivetSetConfig(const std::weak_ptr<Command>& cmd) {
  auto command = cmd.lock();
  if (!command)
    return;
  CHECK(command->GetState() == Command::State::kQueued)
      << EnumToString(command->GetState());
  command->SetProgress(base::DictionaryValue{}, nullptr);

  const auto& settings = device_info_->GetSettings();
  std::string anonymous_access_role{
      EnumToString(settings.local_anonymous_access_role)};
  bool local_access_enabled{settings.local_access_enabled};

  const auto& parameters = command->GetParameters();
  parameters.GetString("maxRoleForAnonymousAccess", &anonymous_access_role);
  parameters.GetBoolean("isLocalAccessEnabled", &local_access_enabled);

  AuthScope auth_scope{AuthScope::kNone};
  if (!StringToEnum(anonymous_access_role, &auth_scope)) {
    ErrorPtr error;
    Error::AddToPrintf(&error, FROM_HERE, errors::commands::kInvalidPropValue,
                       "Invalid maxRoleForAnonymousAccess value '%s'",
                       anonymous_access_role.c_str());
    command->Abort(error.get(), nullptr);
    return;
  }

  device_info_->UpdatePrivetConfig(auth_scope, local_access_enabled);

  command->Complete({}, nullptr);
}

void BaseApiHandler::OnConfigChanged(const Settings& settings) {
  base::DictionaryValue state;
  state.SetString("privet.maxRoleForAnonymousAccess",
                  EnumToString(settings.local_anonymous_access_role));
  state.SetBoolean("privet.isLocalAccessEnabled",
                   settings.local_access_enabled);
  state.SetString("device.name", settings.name);
  state.SetString("device.location", settings.location);
  state.SetString("device.description", settings.description);
  device_->SetStateProperties(kDeviceComponent, state, nullptr);
}

void BaseApiHandler::DeviceSetConfig(const std::weak_ptr<Command>& cmd) {
  auto command = cmd.lock();
  if (!command)
    return;
  CHECK(command->GetState() == Command::State::kQueued)
      << EnumToString(command->GetState());
  command->SetProgress(base::DictionaryValue{}, nullptr);

  const auto& settings = device_info_->GetSettings();
  std::string name{settings.name};
  std::string description{settings.description};
  std::string location{settings.location};

  const auto& parameters = command->GetParameters();
  parameters.GetString("name", &name);
  parameters.GetString("description", &description);
  parameters.GetString("location", &location);

  device_info_->UpdateDeviceInfo(name, description, location);
  command->Complete({}, nullptr);
}

}  // namespace weave
