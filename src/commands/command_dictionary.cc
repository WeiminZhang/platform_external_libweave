// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/commands/command_dictionary.h"

#include <base/values.h>
#include <weave/enum_to_string.h>

#include "src/commands/command_definition.h"
#include "src/commands/schema_constants.h"
#include "src/string_utils.h"

namespace weave {

bool CommandDictionary::LoadCommands(const base::DictionaryValue& json,
                                     ErrorPtr* error) {
  CommandMap new_defs;

  // |json| contains a list of nested objects with the following structure:
  // {"<pkg_name>": {"<cmd_name>": {"parameters": {object_schema}}, ...}, ...}
  // Iterate over packages
  base::DictionaryValue::Iterator package_iter(json);
  while (!package_iter.IsAtEnd()) {
    std::string package_name = package_iter.key();
    const base::DictionaryValue* package_value = nullptr;
    if (!package_iter.value().GetAsDictionary(&package_value)) {
      Error::AddToPrintf(error, FROM_HERE, errors::commands::kDomain,
                         errors::commands::kTypeMismatch,
                         "Expecting an object for package '%s'",
                         package_name.c_str());
      return false;
    }
    // Iterate over command definitions within the current package.
    base::DictionaryValue::Iterator command_iter(*package_value);
    while (!command_iter.IsAtEnd()) {
      std::string command_name = command_iter.key();
      if (command_name.empty()) {
        Error::AddToPrintf(error, FROM_HERE, errors::commands::kDomain,
                           errors::commands::kInvalidCommandName,
                           "Unnamed command encountered in package '%s'",
                           package_name.c_str());
        return false;
      }
      const base::DictionaryValue* command_def_json = nullptr;
      if (!command_iter.value().GetAsDictionary(&command_def_json)) {
        Error::AddToPrintf(error, FROM_HERE, errors::commands::kDomain,
                           errors::commands::kTypeMismatch,
                           "Expecting an object for command '%s'",
                           command_name.c_str());
        return false;
      }
      // Construct the compound command name as "pkg_name.cmd_name".
      std::string full_command_name = Join(".", package_name, command_name);

      auto command_def = CommandDefinition::FromJson(*command_def_json, error);
      if (!command_def) {
        Error::AddToPrintf(error, FROM_HERE, errors::commands::kDomain,
                           errors::commands::kInvalidMinimalRole,
                           "Error parsing command '%s'",
                           full_command_name.c_str());
        return false;
      }

      new_defs.emplace(full_command_name, std::move(command_def));
      command_iter.Advance();
    }
    package_iter.Advance();
  }

  // Verify that newly loaded command definitions do not override existing
  // definitions in another category. This is unlikely, but we don't want to let
  // one vendor daemon to define the same commands already handled by another
  // daemon on the same device.
  for (const auto& pair : new_defs) {
    auto iter = definitions_.find(pair.first);
    CHECK(iter == definitions_.end()) << "Definition for command '"
                                      << pair.first
                                      << "' overrides an earlier definition";
  }

  // Insert new definitions into the global map.
  for (auto& pair : new_defs)
    definitions_.emplace(pair.first, std::move(pair.second));
  return true;
}

std::unique_ptr<base::DictionaryValue> CommandDictionary::GetCommandsAsJson(
    ErrorPtr* error) const {
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue);
  for (const auto& pair : definitions_) {
    auto parts = SplitAtFirst(pair.first, ".", true);
    const std::string& package_name = parts.first;
    const std::string& command_name = parts.second;

    base::DictionaryValue* package = nullptr;
    if (!dict->GetDictionaryWithoutPathExpansion(package_name, &package)) {
      // If this is the first time we encounter this package, create a JSON
      // object for it.
      package = new base::DictionaryValue;
      dict->SetWithoutPathExpansion(package_name, package);
    }
    package->SetWithoutPathExpansion(command_name,
                                     pair.second->ToJson().DeepCopy());
  }
  return dict;
}

const CommandDefinition* CommandDictionary::FindCommand(
    const std::string& command_name) const {
  auto pair = definitions_.find(command_name);
  return (pair != definitions_.end()) ? pair->second.get() : nullptr;
}

CommandDefinition* CommandDictionary::FindCommand(
    const std::string& command_name) {
  auto pair = definitions_.find(command_name);
  return (pair != definitions_.end()) ? pair->second.get() : nullptr;
}

void CommandDictionary::Clear() {
  definitions_.clear();
}

}  // namespace weave
