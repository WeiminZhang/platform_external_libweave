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
#include <weave/error.h>

#include "src/commands/command_definition.h"

namespace base {
class Value;
class DictionaryValue;
}  // namespace base

namespace weave {

// CommandDictionary is a wrapper around a map of command name and the
// corresponding command definition schema. The command name (the key in
// the map) is a compound name in a form of "package_name.command_name",
// where "package_name" is a name of command package such as "base", "printers",
// and others. So the full command name could be "base.reboot", for example.
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
  // Returns empty unique_ptr in case of an error and fills in the additional
  // error details in |error|.
  std::unique_ptr<base::DictionaryValue> GetCommandsAsJson(
      ErrorPtr* error) const;
  // Returns the number of command definitions in the dictionary.
  size_t GetSize() const { return definitions_.size(); }
  // Checks if the dictionary has no command definitions.
  bool IsEmpty() const { return definitions_.empty(); }
  // Remove all the command definitions from the dictionary.
  void Clear();
  // Finds a definition for the given command.
  const CommandDefinition* FindCommand(const std::string& command_name) const;
  CommandDefinition* FindCommand(const std::string& command_name);

 private:
  using CommandMap = std::map<std::string, std::unique_ptr<CommandDefinition>>;

  CommandMap definitions_;  // List of all available command definitions.
  DISALLOW_COPY_AND_ASSIGN(CommandDictionary);
};

}  // namespace weave

#endif  // LIBWEAVE_SRC_COMMANDS_COMMAND_DICTIONARY_H_
