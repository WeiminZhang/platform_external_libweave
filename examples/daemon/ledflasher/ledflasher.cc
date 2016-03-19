// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/daemon/common/daemon.h"

#include <weave/device.h>

#include <base/bind.h>
#include <base/memory/weak_ptr.h>

#include <bitset>

namespace {
const size_t kLedCount = 3;

const char kTraits[] = R"({
  "onOff": {
    "commands": {
      "setConfig": {
        "minimalRole": "user",
        "parameters": {
          "state": {
            "type": "string",
            "enum": [ "on", "off" ]
          }
        }
      }
    },
    "state": {
      "state": {
        "isRequired": true,
        "type": "string",
        "enum": [ "on", "off" ]
      }
    }
  }
})";

const char kLedComponentPrefix[] = "led";
}  // namespace

// LedFlasherHandler is a complete command handler example that shows
// how to handle commands that modify device state.
class LedFlasherHandler {
 public:
  LedFlasherHandler() {}
  void Register(weave::Device* device) {
    device_ = device;

    device->AddTraitDefinitionsFromJson(kTraits);
    for (size_t led_index = 0; led_index < led_states_.size(); led_index++) {
      std::string component_name =
          kLedComponentPrefix + std::to_string(led_index + 1);
      CHECK(device->AddComponent(component_name, {"onOff"}, nullptr));
      device->AddCommandHandler(
          component_name, "onOff.setConfig",
          base::Bind(&LedFlasherHandler::OnOnOffSetConfig,
                     weak_ptr_factory_.GetWeakPtr(), led_index));
      device->SetStateProperty(
          component_name, "onOff.state",
          base::StringValue{led_states_[led_index] ? "on" : "off"}, nullptr);
    }
  }

 private:
  void OnOnOffSetConfig(size_t led_index,
                        const std::weak_ptr<weave::Command>& command) {
    auto cmd = command.lock();
    if (!cmd)
      return;
    LOG(INFO) << "received command: " << cmd->GetName();
    const auto& params = cmd->GetParameters();
    std::string state;
    if (params.GetString("state", &state)) {
      LOG(INFO) << cmd->GetName() << " led: " << led_index
                << " state: " << state;
      int current_state = led_states_[led_index];
      int new_state = (state == "on") ? 1 : 0;
      led_states_[led_index] = new_state;
      if (new_state != current_state) {
        device_->SetStateProperty(cmd->GetComponent(), "onOff.State",
                                  base::StringValue{state}, nullptr);
      }
      cmd->Complete({}, nullptr);
      return;
    }
    weave::ErrorPtr error;
    weave::Error::AddTo(&error, FROM_HERE, "invalid_parameter_value",
                        "Invalid parameters");
    cmd->Abort(error.get(), nullptr);
  }

  weave::Device* device_{nullptr};

  std::bitset<kLedCount> led_states_{0};
  base::WeakPtrFactory<LedFlasherHandler> weak_ptr_factory_{this};
};

int main(int argc, char** argv) {
  Daemon::Options opts;
  if (!opts.Parse(argc, argv)) {
    Daemon::Options::ShowUsage(argv[0]);
    return 1;
  }
  Daemon daemon{opts};
  LedFlasherHandler handler;
  handler.Register(daemon.GetDevice());
  daemon.Run();
  return 0;
}
