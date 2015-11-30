// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_COMMANDS_COMMAND_DICTIONARY_H_
#define LIBWEAVE_SRC_COMMANDS_COMMAND_DICTIONARY_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

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

// CommandDictionary is a wrapper around a container of command definition
// schema. The command name is a compound name in a form of
// "trait_name.command_name", where "trait_name" is a name of command trait such
// as "base", "onOff", and others. So the full command name could be
// "base.reboot", for example.
class CommandDictionary final {
 public:
  CommandDictionary() = default;

  // Loads command definitions from a JSON object. This is done at the daemon
  // startup and whenever a device daemon decides to update its command list.
  // |json| is a JSON dictionary that describes the complete commands. Optional
  // Returns false on failure and |error| provides additional error information
  // when provided.
  bool LoadCommands(const base::DictionaryValue& json,
                    ErrorPtr* error);
  // Converts all the command definitions to a JSON object for CDD/Device
  // draft.
  const base::DictionaryValue& GetCommandsAsJson() const;
  // Returns the number of command definitions in the dictionary.
  size_t GetSize() const;
  // Checks if the dictionary has no command definitions.
  bool IsEmpty() const { return definitions_.empty(); }
  // Remove all the command definitions from the dictionary.
  void Clear();
  // Finds a definition for the given command.
  const base::DictionaryValue* FindCommand(
      const std::string& command_name) const;
  // Determines the minimal role for the given command. Returns false if the
  // command with given name is not found.
  bool GetMinimalRole(const std::string& command_name,
                      UserRole* minimal_role,
                      ErrorPtr* error) const;

 private:
  base::DictionaryValue definitions_;  // List of all available command defs.
  DISALLOW_COPY_AND_ASSIGN(CommandDictionary);
};

}  // namespace weave

#endif  // LIBWEAVE_SRC_COMMANDS_COMMAND_DICTIONARY_H_
