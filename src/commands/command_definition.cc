// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/commands/command_definition.h"

#include <vector>

#include <weave/error.h>
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
}

template <>
LIBWEAVE_EXPORT EnumToStringMap<UserRole>::EnumToStringMap()
    : EnumToStringMap(kMap) {}

CommandDefinition::CommandDefinition(const base::DictionaryValue& definition,
                                     UserRole minimal_role)
    : minimal_role_{minimal_role} {
  definition_.MergeDictionary(&definition);
}

std::unique_ptr<CommandDefinition> CommandDefinition::FromJson(
    const base::DictionaryValue& dict, ErrorPtr* error) {
  std::unique_ptr<CommandDefinition> definition;
  // Validate the 'minimalRole' value if present. That's the only thing we
  // care about so far.
  std::string value;
  UserRole minimal_role;
  if (dict.GetString(commands::attributes::kCommand_Role, &value)) {
    if (!StringToEnum(value, &minimal_role)) {
      Error::AddToPrintf(error, FROM_HERE, errors::commands::kDomain,
                          errors::commands::kInvalidPropValue,
                          "Invalid role: '%s'", value.c_str());
      return definition;
    }
  } else {
    minimal_role = UserRole::kUser;
  }
  definition.reset(new CommandDefinition{dict, minimal_role});
  return definition;
}

}  // namespace weave
