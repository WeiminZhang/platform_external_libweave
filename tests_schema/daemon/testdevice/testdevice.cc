// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <bitset>
#include <string>
#include <typeinfo>

#include "examples/daemon/common/daemon.h"
#include "tests_schema/daemon/testdevice/custom_traits.h"
#include "tests_schema/daemon/testdevice/standard_traits.h"

#include <weave/device.h>
#include <weave/enum_to_string.h>

#include <base/bind.h>
#include <base/memory/weak_ptr.h>

namespace weave {
namespace lockstate {
enum class LockState { kUnlocked, kLocked, kPartiallyLocked };

const weave::EnumToStringMap<LockState>::Map kLockMapMethod[] = {
    {LockState::kLocked, "locked"},
    {LockState::kUnlocked, "unlocked"},
    {LockState::kPartiallyLocked, "partiallyLocked"}};
}  // namespace lockstate

template <>
EnumToStringMap<lockstate::LockState>::EnumToStringMap()
    : EnumToStringMap(lockstate::kLockMapMethod) {}
}  // namespace weave

// TestDeviceHandler is a command handler example that shows
// how to handle commands for a Weave testdevice.
class TestDeviceHandler {
 public:
  TestDeviceHandler() = default;
  void Register(weave::Device* device) {
    device_ = device;

    device->AddTraitDefinitionsFromJson(standard_traits::kTraits);
    device->AddTraitDefinitionsFromJson(custom_traits::kCustomTraits);

    CHECK(device->AddComponent(
        standard_traits::kComponent,
        {"lock", "onOff", "brightness", "colorTemp", "colorXy"}, nullptr));
    CHECK(device->AddComponent(custom_traits::ledflasher, {"_ledflasher"},
                               nullptr));

    CHECK(device->SetStatePropertiesFromJson(
        standard_traits::kComponent, standard_traits::kDefaultState, nullptr));
    CHECK(device->SetStatePropertiesFromJson(
        custom_traits::ledflasher, custom_traits::kLedflasherState, nullptr));

    for (size_t led_index = 0; led_index < led_states_.size(); led_index++) {
      std::string component_name =
          custom_traits::kLedComponentPrefix + std::to_string(led_index + 1);
      CHECK(device->AddComponent(component_name, {"onOff"}, nullptr));
      device->AddCommandHandler(
          component_name, "onOff.setConfig",
          base::Bind(&TestDeviceHandler::LedOnOffSetConfig,
                     weak_ptr_factory_.GetWeakPtr(), led_index));
      device->SetStateProperty(
          component_name, "onOff.state",
          base::StringValue{led_states_[led_index] ? "on" : "off"}, nullptr);
    }

    UpdateTestDeviceState();

    device->AddCommandHandler(standard_traits::kComponent, "onOff.setConfig",
                              base::Bind(&TestDeviceHandler::OnOnOffSetConfig,
                                         weak_ptr_factory_.GetWeakPtr()));
    device->AddCommandHandler(standard_traits::kComponent, "lock.setConfig",
                              base::Bind(&TestDeviceHandler::OnLockSetConfig,
                                         weak_ptr_factory_.GetWeakPtr()));
    device->AddCommandHandler(
        standard_traits::kComponent, "brightness.setConfig",
        base::Bind(&TestDeviceHandler::OnBrightnessSetConfig,
                   weak_ptr_factory_.GetWeakPtr()));
    device->AddCommandHandler(
        standard_traits::kComponent, "colorTemp.setConfig",
        base::Bind(&TestDeviceHandler::OnColorTempSetConfig,
                   weak_ptr_factory_.GetWeakPtr()));
    device->AddCommandHandler(standard_traits::kComponent, "colorXy.setConfig",
                              base::Bind(&TestDeviceHandler::OnColorXySetConfig,
                                         weak_ptr_factory_.GetWeakPtr()));
    device->AddCommandHandler(custom_traits::ledflasher, "_ledflasher.animate",
                              base::Bind(&TestDeviceHandler::OnAnimate,
                                         weak_ptr_factory_.GetWeakPtr()));
  }

 private:
  void OnLockSetConfig(const std::weak_ptr<weave::Command>& command) {
    auto cmd = command.lock();
    if (!cmd)
      return;
    LOG(INFO) << "received command: " << cmd->GetName();
    const auto& params = cmd->GetParameters();
    std::string requested_state;
    if (params.GetString("lockedState", &requested_state)) {
      LOG(INFO) << cmd->GetName() << " state: " << requested_state;

      weave::lockstate::LockState new_lock_status;

      if (!weave::StringToEnum(requested_state, &new_lock_status)) {
        // Invalid lock state was specified.
        AbortCommand(cmd);
        return;
      }

      if (new_lock_status != lock_state_) {
        lock_state_ = new_lock_status;

        LOG(INFO) << "Lock is now: " << requested_state;
        UpdateTestDeviceState();
      }
      cmd->Complete({}, nullptr);
      return;
    }
    AbortCommand(cmd);
  }

  void OnBrightnessSetConfig(const std::weak_ptr<weave::Command>& command) {
    auto cmd = command.lock();
    if (!cmd)
      return;
    LOG(INFO) << "received command: " << cmd->GetName();
    const auto& params = cmd->GetParameters();
    double brightness_value = 0.0;
    if (params.GetDouble("brightness", &brightness_value)) {
      LOG(INFO) << cmd->GetName() << " brightness: " << brightness_value;

      if (brightness_value < 0.0 || brightness_value > 1.0) {
        // Invalid brightness range value is specified.
        AbortCommand(cmd);
        return;
      }

      if (brightness_state_ != brightness_value) {
        brightness_state_ = brightness_value;
        UpdateTestDeviceState();
      }
      cmd->Complete({}, nullptr);
      return;
    }
    AbortCommand(cmd);
  }

  void OnOnOffSetConfig(const std::weak_ptr<weave::Command>& command) {
    auto cmd = command.lock();
    if (!cmd)
      return;
    LOG(INFO) << "received command: " << cmd->GetName();
    const auto& params = cmd->GetParameters();
    std::string requested_state;
    if (params.GetString("state", &requested_state)) {
      LOG(INFO) << cmd->GetName() << " state: " << requested_state;

      std::string temp_state = requested_state;
      std::transform(temp_state.begin(), temp_state.end(), temp_state.begin(),
                     ::toupper);
      if (temp_state != "ON" && temp_state != "OFF") {
        // Invalid OnOff state is specified.
        AbortCommand(cmd);
        return;
      }

      bool new_light_status = requested_state == "on";
      if (new_light_status != light_status_) {
        light_status_ = new_light_status;
        LOG(INFO) << "Light is now: " << (light_status_ ? "ON" : "OFF");
        UpdateTestDeviceState();
      }
      cmd->Complete({}, nullptr);
      return;
    }
    AbortCommand(cmd);
  }

  void OnColorTempSetConfig(const std::weak_ptr<weave::Command>& command) {
    auto cmd = command.lock();
    if (!cmd)
      return;
    LOG(INFO) << "received command: " << cmd->GetName();

    const auto& params = cmd->GetParameters();
    int32_t color_temp = 0;
    if (params.GetInteger("colorTemp", &color_temp)) {
      LOG(INFO) << cmd->GetName() << " colorTemp: " << color_temp;

      if (color_temp < 0.0 || color_temp > 1.0) {
        // Invalid color_temp value is specified.
        AbortCommand(cmd);
        return;
      }

      if (color_temp != color_temp_) {
        color_temp_ = color_temp;

        LOG(INFO) << "color_Temp is now: " << color_temp_;
        UpdateTestDeviceState();
      }
      cmd->Complete({}, nullptr);
      return;
    }

    AbortCommand(cmd);
  }

  void OnColorXySetConfig(const std::weak_ptr<weave::Command>& command) {
    auto cmd = command.lock();
    if (!cmd)
      return;
    LOG(INFO) << "received command: " << cmd->GetName();
    const auto& params = cmd->GetParameters();
    const base::DictionaryValue* colorXy = nullptr;
    if (params.GetDictionary("colorSetting", &colorXy)) {
      bool updateState = false;
      double X = 0.0;
      double Y = 0.0;
      if (colorXy->GetDouble("colorX", &X)) {
        color_X_ = X;
        updateState = true;
      }

      if (colorXy->GetDouble("colorY", &Y)) {
        color_Y_ = Y;
        updateState = true;
      }

      if ((color_X_ < 0.0 || color_Y_ > 1.0) ||
          (color_Y_ < 0.0 || color_Y_ > 1.0)) {
        // Invalid color range value is specified.
        AbortCommand(cmd);
        return;
      }

      if (updateState)
        UpdateTestDeviceState();

      cmd->Complete({}, nullptr);
      return;
    }

    AbortCommand(cmd);
  }

  void OnAnimate(const std::weak_ptr<weave::Command>& command) {
    auto cmd = command.lock();
    if (!cmd)
      return;

    const auto& params = cmd->GetParameters();
    double duration = 0.0;
    std::string type;

    if (params.GetDouble("duration", &duration)) {
      LOG(INFO) << cmd->GetName() << " animate duration : " << duration;

      if (duration <= 0.0 || duration > 100.0) {
        // Invalid animate duration value is specified.
        AbortCommand(cmd);
        return;
      }

      if (params.GetString("type", &type)) {
        LOG(INFO) << " Animation type value is : " << type;
        if (type != "marquee_left" && type != "marquee_right" &&
            type != "blink" && type != "none") {
          // Invalid animation state is specified.
          AbortCommand(cmd);
          return;
        }
      }

      cmd->Complete({}, nullptr);
      return;
    }

    AbortCommand(cmd);
  }

  void LedOnOffSetConfig(size_t led_index,
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

      std::string temp_state = state;
      if (temp_state != "on" && temp_state != "off") {
        // Invalid OnOff state is specified.
        AbortCommand(cmd);
        return;
      }

      int current_state = led_states_[led_index];
      int new_state = (state == "on") ? 1 : 0;
      led_states_[led_index] = new_state;
      if (new_state != current_state) {
        device_->SetStateProperty(cmd->GetComponent(), "onOff.state",
                                  base::StringValue{state}, nullptr);
      }
      cmd->Complete({}, nullptr);
      return;
    }
    AbortCommand(cmd);
  }

  void UpdateTestDeviceState() {
    std::string updated_state = weave::EnumToString(lock_state_);
    device_->SetStateProperty(standard_traits::kComponent, "lock.lockedState",
                              base::StringValue{updated_state}, nullptr);
    base::DictionaryValue state;
    state.SetString("onOff.state", light_status_ ? "on" : "off");
    state.SetDouble("brightness.brightness", brightness_state_);
    state.SetInteger("colorTemp.minColorTemp", color_temp_min_value_);
    state.SetInteger("colorTemp.maxColorTemp", color_temp_max_value_);
    state.SetInteger("colorTemp.colorTemp", color_temp_);

    std::unique_ptr<base::DictionaryValue> colorXy(new base::DictionaryValue());
    colorXy->SetDouble("colorX", color_X_);
    colorXy->SetDouble("colorY", color_Y_);
    state.Set("colorXy.colorSetting", std::move(colorXy));

    device_->SetStateProperties(standard_traits::kComponent, state, nullptr);
  }

  void AbortCommand(std::shared_ptr<weave::Command>& cmd) {
    weave::ErrorPtr error;
    weave::Error::AddTo(&error, FROM_HERE, "invalidParameterValue",
                        "Invalid parameters");
    cmd->Abort(error.get(), nullptr);
  }

  weave::Device* device_{nullptr};

  // Simulate the state of the testdevice.
  weave::lockstate::LockState lock_state_{weave::lockstate::LockState::kLocked};
  bool light_status_{false};
  double brightness_state_{0.0};
  int32_t color_temp_{0};
  int32_t color_temp_min_value_{0};
  int32_t color_temp_max_value_{1};
  double color_X_{0.0};
  double color_Y_{0.0};
  double duration{0.1};
  std::bitset<custom_traits::kLedCount> led_states_{0};
  base::WeakPtrFactory<TestDeviceHandler> weak_ptr_factory_{this};
};

int main(int argc, char** argv) {
  Daemon::Options opts;
  opts.model_id = "AOAAA";
  if (!opts.Parse(argc, argv)) {
    Daemon::Options::ShowUsage(argv[0]);
    return 1;
  }
  Daemon daemon{opts};
  TestDeviceHandler handler;
  handler.Register(daemon.GetDevice());
  daemon.Run();
  return 0;
}
