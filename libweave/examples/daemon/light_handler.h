// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <weave/device.h>

#include <base/bind.h>
#include <base/memory/weak_ptr.h>

namespace weave {
namespace examples {
namespace daemon {

// LightHandler is a command handler example that shows
// how to handle commands for a Weave light.
class LightHandler {
 public:
  LightHandler() = default;
  void Register(Device* device) {
    device_ = device;

    device->AddStateDefinitionsFromJson(R"({
      "onOff": {"state": ["on", "standby"]},
      "brightness": {"brightness": "integer"}
    })");

    device->SetStatePropertiesFromJson(R"({
      "onOff":{"state": "standby"},
      "brightness":{"brightness": 0}
    })",
                                       nullptr);

    // Once bug b/25304415 is fixed, these should be changed
    // to use the standard commands.
    device->AddCommandDefinitionsFromJson(R"({
      "onOff": {
         "_setConfig":{
           "parameters": {
             "_state": ["on", "standby"]
           }
         }
       },
       "brightness": {
         "_setConfig":{
           "parameters": {
             "_brightness": {
               "type": "integer",
               "minimum": 0,
               "maximum": 100
             }
           }
        }
      }
    })");
    device->AddCommandHandler(
        "onOff._setConfig",
        base::Bind(&LightHandler::OnOnOffSetConfig,
                   weak_ptr_factory_.GetWeakPtr()));
    device->AddCommandHandler(
        "brightness._setConfig",
         base::Bind(&LightHandler::OnBrightnessSetConfig,
                    weak_ptr_factory_.GetWeakPtr()));
  }

 private:
  void OnBrightnessSetConfig(const std::weak_ptr<Command>& command) {
    auto cmd = command.lock();
    if (!cmd)
      return;
    LOG(INFO) << "received command: " << cmd->GetName();
    int32_t brightness_value = 0;
    if (cmd->GetParameters()->GetInteger("_brightness", &brightness_value)) {
      // Display this command in terminal.
      LOG(INFO) << cmd->GetName() << " brightness: " << brightness_value;

      if (brightness_state_ != brightness_value) {
        brightness_state_ = brightness_value;
        UpdateLightState();
      }
      cmd->Complete({}, nullptr);
      return;
    }
    ErrorPtr error;
    Error::AddTo(&error, FROM_HERE, "example", "invalid_parameter_value",
                 "Invalid parameters");
    cmd->Abort(error.get(), nullptr);
  }

  void OnOnOffSetConfig(const std::weak_ptr<Command>& command) {
    auto cmd = command.lock();
    if (!cmd)
      return;
    LOG(INFO) << "received command: " << cmd->GetName();
    std::string requested_state;
    if (cmd->GetParameters()->GetString("_state", &requested_state)) {
      LOG(INFO) << cmd->GetName() << " state: " << requested_state;

      bool new_light_status = requested_state == "on";
      if (new_light_status != light_status_) {
        light_status_ = new_light_status;

        LOG(INFO) << "Light is now: " << (light_status_ ? "ON" : "OFF");
        UpdateLightState();
      }
      cmd->Complete({}, nullptr);
      return;
    }
    ErrorPtr error;
    Error::AddTo(&error, FROM_HERE, "example", "invalid_parameter_value",
                 "Invalid parameters");
    cmd->Abort(error.get(), nullptr);
  }

  void UpdateLightState(void) {
    base::DictionaryValue state;
    state.SetString("onOff.state", light_status_ ? "on" : "standby");
    state.SetInteger("brightness.brightness", brightness_state_);
    device_->SetStateProperties(state, nullptr);
  }

  Device* device_{nullptr};

  // Simulate the state of the light.
  bool light_status_;
  int32_t brightness_state_;
  base::WeakPtrFactory<LightHandler> weak_ptr_factory_{this};
};

}  // namespace daemon
}  // namespace examples
}  // namespace weave
