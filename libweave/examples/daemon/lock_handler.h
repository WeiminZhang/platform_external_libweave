// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

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

namespace examples {
namespace daemon {

// LockHandler is a command handler example that shows
// how to handle commands for a Weave lock.
class LockHandler {
 public:
  LockHandler() = default;
  void Register(Device* device) {
    device_ = device;

    device->AddStateDefinitionsFromJson(R"({
      "_lock": {"lockedState": ["locked", "unlocked", "partiallyLocked"]}
    })");

    device->SetStatePropertiesFromJson(R"({
      "_lock":{"lockedState": "locked"}
    })",
                                       nullptr);

    // Once bug b/25304415 is fixed, and when the lock trait is published
    // these should be changed  to use the standard commands
    device->AddCommandDefinitionsFromJson(R"({
        "_lock": {
          "_setConfig":{
            "parameters": {
              "_lockedState": ["locked", "unlocked"]
            }
          }
        }
    })");
    device->AddCommandHandler(
        "_lock._setConfig",
        base::Bind(&LockHandler::OnLockSetConfig,
                   weak_ptr_factory_.GetWeakPtr()));
  }

 private:
  void OnLockSetConfig(const std::weak_ptr<Command>& command) {
    auto cmd = command.lock();
    if (!cmd)
      return;
    LOG(INFO) << "received command: " << cmd->GetName();
    std::string requested_state;
    if (cmd->GetParameters()->GetString("_lockedState", &requested_state)) {
      LOG(INFO) << cmd->GetName() << " state: " << requested_state;

      lockstate::LockState new_lock_status;

      if (!weave::StringToEnum(requested_state, &new_lock_status)) {
        // Invalid lock state was specified.
        ErrorPtr error;
        Error::AddTo(&error, FROM_HERE, "example", "invalid_parameter_value",
                     "Invalid parameters");
        cmd->Abort(error.get(), nullptr);
        return;
      }

      if (new_lock_status != lock_state_) {
        lock_state_ = new_lock_status;

        LOG(INFO) << "Lock is now: " << requested_state;
        UpdateLockState();
      }
      cmd->Complete({}, nullptr);
      return;
    }
    ErrorPtr error;
    Error::AddTo(&error, FROM_HERE, "example", "invalid_parameter_value",
                 "Invalid parameters");
    cmd->Abort(error.get(), nullptr);
  }

  void UpdateLockState(void) {
    base::DictionaryValue state;
    std::string updated_state = weave::EnumToString(lock_state_);
    state.SetString("lock.lockedState", updated_state);
    device_->SetStateProperties(state, nullptr);
  }

  Device* device_{nullptr};

  // Simulate the state of the light.
  lockstate::LockState lock_state_{lockstate::LockState::kLocked};
  base::WeakPtrFactory<LockHandler> weak_ptr_factory_{this};
};

}  // namespace daemon
}  // namespace examples
}  // namespace weave
