// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/commands/command_dictionary.h"

#include <algorithm>

#include <base/values.h>
#include <weave/enum_to_string.h>

#include "src/commands/schema_constants.h"
#include "src/string_utils.h"

namespace weave {

namespace {
const EnumToStringMap<UserRole>::Map kMap[] = {
    {UserRole::kViewer, commands::attributes::kCommand_Role_Viewer},
    {UserRole::kUser, commands::attributes::kCommand_Role_User},
    {UserRole::kOwner, commands::attributes::kCommand_Role_Owner},
    {UserRole::kManager, commands::attributes::kCommand_Role_Manager},
};
}  // anonymous namespace

template <>
LIBWEAVE_EXPORT EnumToStringMap<UserRole>::EnumToStringMap()
    : EnumToStringMap(kMap) {}

bool CommandDictionary::LoadCommands(const base::DictionaryValue& json,
                                     ErrorPtr* error) {
  // |json| contains a list of nested objects with the following structure:
  // {"<pkg_name>": {"<cmd_name>": {"parameters": {object_schema}}, ...}, ...}
  // Iterate over traits
  base::DictionaryValue::Iterator trait_iter(json);
  for (base::DictionaryValue::Iterator trait_iter(json);
       !trait_iter.IsAtEnd(); trait_iter.Advance()) {
    std::string trait_name = trait_iter.key();
    const base::DictionaryValue* trait_def = nullptr;
    if (!trait_iter.value().GetAsDictionary(&trait_def)) {
      Error::AddToPrintf(error, FROM_HERE, errors::commands::kDomain,
                         errors::commands::kTypeMismatch,
                         "Expecting an object for trait '%s'",
                         trait_name.c_str());
      return false;
    }
    // Iterate over command definitions within the current trait.
    for (base::DictionaryValue::Iterator command_iter(*trait_def);
         !command_iter.IsAtEnd(); command_iter.Advance()) {
      std::string command_name = command_iter.key();
      if (command_name.empty()) {
        Error::AddToPrintf(error, FROM_HERE, errors::commands::kDomain,
                           errors::commands::kInvalidCommandName,
                           "Unnamed command encountered in trait '%s'",
                           trait_name.c_str());
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

      // Construct the compound command name as "trait_name.cmd_name".
      std::string full_command_name = Join(".", trait_name, command_name);

      // Validate the 'minimalRole' value if present. That's the only thing we
      // care about so far.
      std::string value;
      if (!command_def_json->GetString(commands::attributes::kCommand_Role,
                                       &value)) {
        Error::AddToPrintf(error, FROM_HERE, errors::commands::kDomain,
                           errors::commands::kInvalidMinimalRole,
                           "Missing '%s' attribute for command '%s'",
                           commands::attributes::kCommand_Role,
                           full_command_name.c_str());
        return false;
      }
      UserRole minimal_role;
      if (!StringToEnum(value, &minimal_role)) {
        Error::AddToPrintf(error, FROM_HERE, errors::commands::kDomain,
                           errors::commands::kInvalidMinimalRole,
                           "Invalid role '%s' for command '%s'", value.c_str(),
                           full_command_name.c_str());
        return false;
      }
      // Check if we already have this command defined.
      CHECK(!definitions_.Get(full_command_name, nullptr))
          << "Definition for command '" << full_command_name
          << "' overrides an earlier definition";
      definitions_.Set(full_command_name, command_def_json->DeepCopy());
    }
  }
  return true;
}

const base::DictionaryValue& CommandDictionary::GetCommandsAsJson() const {
  return definitions_;
}

size_t CommandDictionary::GetSize() const {
  size_t size = 0;
  base::DictionaryValue::Iterator trait_iter(definitions_);
  while (!trait_iter.IsAtEnd()) {
    std::string trait_name = trait_iter.key();
    const base::DictionaryValue* trait_def = nullptr;
    CHECK(trait_iter.value().GetAsDictionary(&trait_def));
    size += trait_def->size();
    trait_iter.Advance();
  }
  return size;
}

const base::DictionaryValue* CommandDictionary::FindCommand(
    const std::string& command_name) const {
  const base::DictionaryValue* definition = nullptr;
  // Make sure the |command_name| came in form of trait_name.command_name.
  // For this, we just verify it has a single period in its name.
  if (std::count(command_name.begin(), command_name.end(), '.') != 1)
    return definition;
  definitions_.GetDictionary(command_name, &definition);
  return definition;
}

void CommandDictionary::Clear() {
  definitions_.Clear();
}

bool CommandDictionary::GetMinimalRole(const std::string& command_name,
                                       UserRole* minimal_role,
                                       ErrorPtr* error) const {
  const base::DictionaryValue* command_def = FindCommand(command_name);
  if (!command_def) {
    Error::AddToPrintf(error, FROM_HERE, errors::commands::kDomain,
                        errors::commands::kInvalidCommandName,
                        "Command definition for '%s' not found",
                        command_name.c_str());
    return false;
  }
  std::string value;
  // The JSON definition has been pre-validated already in LoadCommands, so
  // just using CHECKs here.
  CHECK(command_def->GetString(commands::attributes::kCommand_Role, &value));
  CHECK(StringToEnum(value, minimal_role));
  return true;
}

}  // namespace weave
