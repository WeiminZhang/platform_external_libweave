// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/commands/command_manager.h"

#include <base/values.h>
#include <weave/enum_to_string.h>
#include <weave/error.h>

#include "src/commands/schema_constants.h"
#include "src/utils.h"

namespace weave {

CommandManager::CommandManager() {}

CommandManager::~CommandManager() {}

void CommandManager::AddCommandDefChanged(const base::Closure& callback) {
  on_command_changed_.push_back(callback);
  callback.Run();
}

const CommandDictionary& CommandManager::GetCommandDictionary() const {
  return dictionary_;
}

bool CommandManager::LoadCommands(const base::DictionaryValue& dict,
                                  ErrorPtr* error) {
  bool result = dictionary_.LoadCommands(dict, error);
  for (const auto& cb : on_command_changed_)
    cb.Run();
  return result;
}

bool CommandManager::LoadCommands(const std::string& json,
                                  ErrorPtr* error) {
  std::unique_ptr<const base::DictionaryValue> dict = LoadJsonDict(json, error);
  if (!dict)
    return false;
  return LoadCommands(*dict, error);
}

void CommandManager::AddCommand(
    std::unique_ptr<CommandInstance> command_instance) {
  command_queue_.Add(std::move(command_instance));
}

bool CommandManager::AddCommand(const base::DictionaryValue& command,
                                std::string* id,
                                ErrorPtr* error) {
  return AddCommand(command, UserRole::kOwner, id, error);
}

bool CommandManager::AddCommand(const base::DictionaryValue& command,
                                UserRole role,
                                std::string* id,
                                ErrorPtr* error) {
  auto command_instance = CommandInstance::FromJson(
      &command, Command::Origin::kLocal, nullptr, error);
  if (!command_instance)
    return false;

  UserRole minimal_role;
  if (!GetCommandDictionary().GetMinimalRole(command_instance->GetName(),
                                             &minimal_role, error)) {
    return false;
  }
  if (role < minimal_role) {
    Error::AddToPrintf(
        error, FROM_HERE, errors::commands::kDomain, "access_denied",
        "User role '%s' less than minimal: '%s'", EnumToString(role).c_str(),
        EnumToString(minimal_role).c_str());
    return false;
  }

  command_instance->SetComponent("device");
  *id = std::to_string(++next_command_id_);
  command_instance->SetID(*id);
  AddCommand(std::move(command_instance));
  return true;
}

CommandInstance* CommandManager::FindCommand(const std::string& id) {
  return command_queue_.Find(id);
}

void CommandManager::AddCommandAddedCallback(
    const CommandQueue::CommandCallback& callback) {
  command_queue_.AddCommandAddedCallback(callback);
}

void CommandManager::AddCommandRemovedCallback(
    const CommandQueue::CommandCallback& callback) {
  command_queue_.AddCommandRemovedCallback(callback);
}

void CommandManager::AddCommandHandler(
    const std::string& command_name,
    const Device::CommandHandlerCallback& callback) {
  CHECK(command_name.empty() || dictionary_.FindCommand(command_name))
      << "Command undefined: " << command_name;
  command_queue_.AddCommandHandler("device", command_name, callback);
}

}  // namespace weave
