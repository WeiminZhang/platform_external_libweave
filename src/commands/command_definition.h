// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_COMMANDS_COMMAND_DEFINITION_H_
#define LIBWEAVE_SRC_COMMANDS_COMMAND_DEFINITION_H_

#include <memory>
#include <string>

#include <base/macros.h>
#include <base/values.h>
#include <weave/error.h>

namespace weave {

enum class UserRole {
  kViewer,
  kUser,
  kManager,
  kOwner,
};

// A simple GCD command definition. This class contains the full object schema
// describing the command parameter types and constraints.
class CommandDefinition final {
 public:
  // Factory method to construct a command definition from a JSON dictionary.
  static std::unique_ptr<CommandDefinition> FromJson(
      const base::DictionaryValue& dict, ErrorPtr* error);
  const base::DictionaryValue& ToJson() const { return definition_; }
  // Returns the role required to execute command.
  UserRole GetMinimalRole() const { return minimal_role_; }

 private:
  CommandDefinition(const base::DictionaryValue& definition,
                    UserRole minimal_role);

  base::DictionaryValue definition_;
  UserRole minimal_role_;

  DISALLOW_COPY_AND_ASSIGN(CommandDefinition);
};

}  // namespace weave

#endif  // LIBWEAVE_SRC_COMMANDS_COMMAND_DEFINITION_H_
