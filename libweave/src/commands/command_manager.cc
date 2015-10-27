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

namespace {

const char kStandardCommandDefs[] = R"({
  "base": {
    "updateBaseConfiguration": {
      "minimalRole": "manager",
      "parameters": {
        "localDiscoveryEnabled": "boolean",
        "localAnonymousAccessMaxRole": [ "none", "viewer", "user" ],
        "localPairingEnabled": "boolean"
      },
      "results": {}
    },
    "reboot": {
      "minimalRole": "user",
      "parameters": {},
      "results": {}
    },
    "identify": {
      "minimalRole": "user",
      "parameters": {},
      "results": {}
    },
    "updateDeviceInfo": {
      "minimalRole": "manager",
      "parameters": {
        "description": "string",
        "name": "string",
        "location": "string"
      },
      "results": {}
    }
  }
})";

}  // namespace

CommandManager::CommandManager() {}

CommandManager::~CommandManager() {}

void CommandManager::AddCommandDefChanged(const base::Closure& callback) {
  on_command_changed_.push_back(callback);
  callback.Run();
}

const CommandDictionary& CommandManager::GetCommandDictionary() const {
  return dictionary_;
}

bool CommandManager::LoadStandardCommands(const base::DictionaryValue& dict,
                                          ErrorPtr* error) {
  return standard_dictionary_.LoadCommands(dict, nullptr, error);
}

bool CommandManager::LoadStandardCommands(const std::string& json,
                                          ErrorPtr* error) {
  std::unique_ptr<const base::DictionaryValue> dict = LoadJsonDict(json, error);
  if (!dict)
    return false;
  return LoadStandardCommands(*dict, error);
}

bool CommandManager::LoadCommands(const base::DictionaryValue& dict,
                                  ErrorPtr* error) {
  bool result = dictionary_.LoadCommands(dict, &standard_dictionary_, error);
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

void CommandManager::Startup() {
  LOG(INFO) << "Initializing CommandManager.";

  // Load global standard GCD command dictionary.
  CHECK(LoadStandardCommands(kStandardCommandDefs, nullptr));
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
  auto command_instance =
      CommandInstance::FromJson(&command, Command::Origin::kLocal,
                                GetCommandDictionary(), nullptr, error);
  if (!command_instance)
    return false;

  UserRole minimal_role =
      command_instance->GetCommandDefinition()->GetMinimalRole();
  if (role < minimal_role) {
    Error::AddToPrintf(
        error, FROM_HERE, errors::commands::kDomain, "access_denied",
        "User role '%s' less than minimal: '%s'", EnumToString(role).c_str(),
        EnumToString(minimal_role).c_str());
    return false;
  }

  *id = std::to_string(++next_command_id_);
  command_instance->SetID(*id);
  AddCommand(std::move(command_instance));
  return true;
}

CommandInstance* CommandManager::FindCommand(const std::string& id) {
  return command_queue_.Find(id);
}

bool CommandManager::SetCommandVisibility(
    const std::vector<std::string>& command_names,
    CommandDefinition::Visibility visibility,
    ErrorPtr* error) {
  if (command_names.empty())
    return true;

  std::vector<CommandDefinition*> definitions;
  definitions.reserve(command_names.size());

  // Find/validate command definitions first.
  for (const std::string& name : command_names) {
    CommandDefinition* def = dictionary_.FindCommand(name);
    if (!def) {
      Error::AddToPrintf(error, FROM_HERE, errors::commands::kDomain,
                         errors::commands::kInvalidCommandName,
                         "Command '%s' is unknown", name.c_str());
      return false;
    }
    definitions.push_back(def);
  }

  // Now that we know that all the command names were valid,
  // update the respective commands' visibility.
  for (CommandDefinition* def : definitions)
    def->SetVisibility(visibility);
  for (const auto& cb : on_command_changed_)
    cb.Run();
  return true;
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
  command_queue_.AddCommandHandler(command_name, callback);
}

}  // namespace weave
