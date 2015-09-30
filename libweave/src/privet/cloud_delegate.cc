// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/privet/cloud_delegate.h"

#include <map>
#include <vector>

#include <base/bind.h>
#include <base/logging.h>
#include <base/memory/weak_ptr.h>
#include <base/values.h>
#include <weave/error.h>
#include <weave/provider/task_runner.h>

#include "src/backoff_entry.h"
#include "src/commands/command_manager.h"
#include "src/config.h"
#include "src/device_registration_info.h"
#include "src/privet/constants.h"
#include "src/states/state_manager.h"

namespace weave {
namespace privet {

namespace {

const BackoffEntry::Policy register_backoff_policy = {0,    1000, 2.0,  0.2,
                                                      5000, -1,   false};

const int kMaxDeviceRegistrationTimeMinutes = 5;

Command* ReturnNotFound(const std::string& command_id, ErrorPtr* error) {
  Error::AddToPrintf(error, FROM_HERE, errors::kDomain, errors::kNotFound,
                     "Command not found, ID='%s'", command_id.c_str());
  return nullptr;
}

class CloudDelegateImpl : public CloudDelegate {
 public:
  CloudDelegateImpl(provider::TaskRunner* task_runner,
                    DeviceRegistrationInfo* device,
                    CommandManager* command_manager,
                    StateManager* state_manager)
      : task_runner_{task_runner},
        device_{device},
        command_manager_{command_manager},
        state_manager_{state_manager} {
    device_->AddOnConfigChangedCallback(base::Bind(
        &CloudDelegateImpl::OnConfigChanged, weak_factory_.GetWeakPtr()));
    device_->AddOnRegistrationChangedCallback(base::Bind(
        &CloudDelegateImpl::OnRegistrationChanged, weak_factory_.GetWeakPtr()));

    command_manager_->AddOnCommandDefChanged(base::Bind(
        &CloudDelegateImpl::OnCommandDefChanged, weak_factory_.GetWeakPtr()));
    command_manager_->AddOnCommandAddedCallback(base::Bind(
        &CloudDelegateImpl::OnCommandAdded, weak_factory_.GetWeakPtr()));
    command_manager_->AddOnCommandRemovedCallback(base::Bind(
        &CloudDelegateImpl::OnCommandRemoved, weak_factory_.GetWeakPtr()));

    state_manager_->AddOnChangedCallback(base::Bind(
        &CloudDelegateImpl::OnStateChanged, weak_factory_.GetWeakPtr()));
  }

  ~CloudDelegateImpl() override = default;

  std::string GetModelId() const override {
    CHECK_EQ(5u, device_->GetSettings().model_id.size());
    return device_->GetSettings().model_id;
  }

  std::string GetName() const override { return device_->GetSettings().name; }

  std::string GetDescription() const override {
    return device_->GetSettings().description;
  }

  std::string GetLocation() const override {
    return device_->GetSettings().location;
  }

  void UpdateDeviceInfo(const std::string& name,
                        const std::string& description,
                        const std::string& location) override {
    device_->UpdateDeviceInfo(name, description, location);
  }

  std::string GetOemName() const override {
    return device_->GetSettings().oem_name;
  }

  std::string GetModelName() const override {
    return device_->GetSettings().model_name;
  }

  std::set<std::string> GetServices() const override {
    std::set<std::string> result;
    for (base::DictionaryValue::Iterator it{command_defs_}; !it.IsAtEnd();
         it.Advance()) {
      result.emplace(it.key());
    }
    return result;
  }

  AuthScope GetAnonymousMaxScope() const override {
    return device_->GetSettings().local_anonymous_access_role;
  }

  const ConnectionState& GetConnectionState() const override {
    return connection_state_;
  }

  const SetupState& GetSetupState() const override { return setup_state_; }

  bool Setup(const std::string& ticket_id,
             const std::string& user,
             ErrorPtr* error) override {
    if (setup_state_.IsStatusEqual(SetupState::kInProgress)) {
      Error::AddTo(error, FROM_HERE, errors::kDomain, errors::kDeviceBusy,
                   "Setup in progress");
      return false;
    }
    VLOG(1) << "GCD Setup started. ticket_id: " << ticket_id
            << ", user:" << user;
    setup_state_ = SetupState(SetupState::kInProgress);
    setup_weak_factory_.InvalidateWeakPtrs();
    backoff_entry_.Reset();
    base::Time deadline = base::Time::Now();
    deadline += base::TimeDelta::FromMinutes(kMaxDeviceRegistrationTimeMinutes);
    task_runner_->PostDelayedTask(
        FROM_HERE,
        base::Bind(&CloudDelegateImpl::CallManagerRegisterDevice,
                   setup_weak_factory_.GetWeakPtr(), ticket_id, deadline),
        {});
    // Return true because we initiated setup.
    return true;
  }

  std::string GetCloudId() const override {
    return device_->GetSettings().cloud_id;
  }

  const base::DictionaryValue& GetState() const override { return state_; }

  const base::DictionaryValue& GetCommandDef() const override {
    return command_defs_;
  }

  void AddCommand(const base::DictionaryValue& command,
                  const UserInfo& user_info,
                  const CommandSuccessCallback& success_callback,
                  const ErrorCallback& error_callback) override {
    CHECK(user_info.scope() != AuthScope::kNone);
    CHECK_NE(user_info.user_id(), 0u);

    ErrorPtr error;
    UserRole role;
    std::string str_scope = EnumToString(user_info.scope());
    if (!StringToEnum(str_scope, &role)) {
      Error::AddToPrintf(&error, FROM_HERE, errors::kDomain,
                         errors::kInvalidParams, "Invalid role: '%s'",
                         str_scope.c_str());
      return error_callback.Run(error.get());
    }

    std::string id;
    if (!command_manager_->AddCommand(command, role, &id, &error))
      return error_callback.Run(error.get());

    command_owners_[id] = user_info.user_id();
    success_callback.Run(*command_manager_->FindCommand(id)->ToJson());
  }

  void GetCommand(const std::string& id,
                  const UserInfo& user_info,
                  const CommandSuccessCallback& success_callback,
                  const ErrorCallback& error_callback) override {
    CHECK(user_info.scope() != AuthScope::kNone);
    ErrorPtr error;
    auto command = GetCommandInternal(id, user_info, &error);
    if (!command)
      return error_callback.Run(error.get());
    success_callback.Run(*command->ToJson());
  }

  void CancelCommand(const std::string& id,
                     const UserInfo& user_info,
                     const CommandSuccessCallback& success_callback,
                     const ErrorCallback& error_callback) override {
    CHECK(user_info.scope() != AuthScope::kNone);
    ErrorPtr error;
    auto command = GetCommandInternal(id, user_info, &error);
    if (!command)
      return error_callback.Run(error.get());

    command->Cancel();
    success_callback.Run(*command->ToJson());
  }

  void ListCommands(const UserInfo& user_info,
                    const CommandSuccessCallback& success_callback,
                    const ErrorCallback& error_callback) override {
    CHECK(user_info.scope() != AuthScope::kNone);

    base::ListValue list_value;

    for (const auto& it : command_owners_) {
      if (CanAccessCommand(it.second, user_info, nullptr)) {
        list_value.Append(
            command_manager_->FindCommand(it.first)->ToJson().release());
      }
    }

    base::DictionaryValue commands_json;
    commands_json.Set("commands", list_value.DeepCopy());

    success_callback.Run(commands_json);
  }

 private:
  void OnCommandAdded(Command* command) {
    // Set to 0 for any new unknown command.
    command_owners_.emplace(command->GetID(), 0);
  }

  void OnCommandRemoved(Command* command) {
    CHECK(command_owners_.erase(command->GetID()));
  }

  void OnConfigChanged(const Settings&) { NotifyOnDeviceInfoChanged(); }

  void OnRegistrationChanged(RegistrationStatus status) {
    if (status == RegistrationStatus::kUnconfigured) {
      connection_state_ = ConnectionState{ConnectionState::kUnconfigured};
    } else if (status == RegistrationStatus::kConnecting) {
      // TODO(vitalybuka): Find conditions for kOffline.
      connection_state_ = ConnectionState{ConnectionState::kConnecting};
    } else if (status == RegistrationStatus::kConnected) {
      connection_state_ = ConnectionState{ConnectionState::kOnline};
    } else {
      ErrorPtr error;
      Error::AddToPrintf(
          &error, FROM_HERE, errors::kDomain, errors::kInvalidState,
          "Unexpected registration status: %s", EnumToString(status).c_str());
      connection_state_ = ConnectionState{std::move(error)};
    }
    NotifyOnDeviceInfoChanged();
  }

  void OnStateChanged() {
    state_.Clear();
    auto state = state_manager_->GetStateValuesAsJson();
    CHECK(state);
    state_.MergeDictionary(state.get());
    NotifyOnStateChanged();
  }

  void OnCommandDefChanged() {
    command_defs_.Clear();
    auto commands = command_manager_->GetCommandDictionary().GetCommandsAsJson(
        [](const CommandDefinition* def) { return def->GetVisibility().local; },
        true, nullptr);
    CHECK(commands);
    command_defs_.MergeDictionary(commands.get());
    NotifyOnCommandDefsChanged();
  }

  void OnRegisterSuccess(const std::string& cloud_id) {
    VLOG(1) << "Device registered: " << cloud_id;
    setup_state_ = SetupState(SetupState::kSuccess);
  }

  void CallManagerRegisterDevice(const std::string& ticket_id,
                                 const base::Time& deadline) {
    ErrorPtr error;
    if (base::Time::Now() > deadline) {
      Error::AddTo(&error, FROM_HERE, errors::kDomain, errors::kInvalidState,
                   "Failed to register device");
      setup_state_ = SetupState{std::move(error)};
      return;
    }

    if (!device_->RegisterDevice(ticket_id, &error).empty()) {
      backoff_entry_.InformOfRequest(true);
      setup_state_ = SetupState(SetupState::kSuccess);
      return;
    }

    // Registration failed. Retry with backoff.
    backoff_entry_.InformOfRequest(false);
    task_runner_->PostDelayedTask(
        FROM_HERE,
        base::Bind(&CloudDelegateImpl::CallManagerRegisterDevice,
                   setup_weak_factory_.GetWeakPtr(), ticket_id, deadline),
        backoff_entry_.GetTimeUntilRelease());
  }

  Command* GetCommandInternal(const std::string& command_id,
                              const UserInfo& user_info,
                              ErrorPtr* error) const {
    if (user_info.scope() != AuthScope::kOwner) {
      auto it = command_owners_.find(command_id);
      if (it == command_owners_.end())
        return ReturnNotFound(command_id, error);
      if (CanAccessCommand(it->second, user_info, error))
        return nullptr;
    }

    auto command = command_manager_->FindCommand(command_id);
    if (!command)
      return ReturnNotFound(command_id, error);

    return command;
  }

  bool CanAccessCommand(uint64_t owner_id,
                        const UserInfo& user_info,
                        ErrorPtr* error) const {
    CHECK(user_info.scope() != AuthScope::kNone);
    CHECK_NE(user_info.user_id(), 0u);

    if (user_info.scope() == AuthScope::kOwner ||
        owner_id == user_info.user_id()) {
      return true;
    }

    Error::AddTo(error, FROM_HERE, errors::kDomain, errors::kAccessDenied,
                 "Need to be owner of the command.");
    return false;
  }

  provider::TaskRunner* task_runner_{nullptr};
  DeviceRegistrationInfo* device_{nullptr};
  CommandManager* command_manager_{nullptr};
  StateManager* state_manager_{nullptr};

  // Primary state of GCD.
  ConnectionState connection_state_{ConnectionState::kDisabled};

  // State of the current or last setup.
  SetupState setup_state_{SetupState::kNone};

  // Current device state.
  base::DictionaryValue state_;

  // Current commands definitions.
  base::DictionaryValue command_defs_;

  // Map of command IDs to user IDs.
  std::map<std::string, uint64_t> command_owners_;

  // Backoff entry for retrying device registration.
  BackoffEntry backoff_entry_{&register_backoff_policy};

  // |setup_weak_factory_| tracks the lifetime of callbacks used in connection
  // with a particular invocation of Setup().
  base::WeakPtrFactory<CloudDelegateImpl> setup_weak_factory_{this};
  // |weak_factory_| tracks the lifetime of |this|.
  base::WeakPtrFactory<CloudDelegateImpl> weak_factory_{this};
};

}  // namespace

CloudDelegate::CloudDelegate() {}

CloudDelegate::~CloudDelegate() {}

// static
std::unique_ptr<CloudDelegate> CloudDelegate::CreateDefault(
    provider::TaskRunner* task_runner,
    DeviceRegistrationInfo* device,
    CommandManager* command_manager,
    StateManager* state_manager) {
  return std::unique_ptr<CloudDelegateImpl>{new CloudDelegateImpl{
      task_runner, device, command_manager, state_manager}};
}

void CloudDelegate::NotifyOnDeviceInfoChanged() {
  FOR_EACH_OBSERVER(Observer, observer_list_, OnDeviceInfoChanged());
}

void CloudDelegate::NotifyOnCommandDefsChanged() {
  FOR_EACH_OBSERVER(Observer, observer_list_, OnCommandDefsChanged());
}

void CloudDelegate::NotifyOnStateChanged() {
  FOR_EACH_OBSERVER(Observer, observer_list_, OnStateChanged());
}

}  // namespace privet
}  // namespace weave
