// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_COMMANDS_COMMAND_MANAGER_H_
#define LIBWEAVE_SRC_COMMANDS_COMMAND_MANAGER_H_

#include <memory>
#include <string>
#include <vector>

#include <base/callback.h>
#include <base/macros.h>
#include <base/memory/weak_ptr.h>

#include "src/commands/command_dictionary.h"
#include "src/commands/command_queue.h"

namespace weave {

class CommandInstance;

// CommandManager class that will have a list of all the device command
// schemas as well as the live command queue of pending command instances
// dispatched to the device.
class CommandManager final {
 public:
  CommandManager();

  ~CommandManager();

  bool AddCommand(const base::DictionaryValue& command,
                  std::string* id,
                  ErrorPtr* error);
  CommandInstance* FindCommand(const std::string& id);
  void AddCommandAddedCallback(const CommandQueue::CommandCallback& callback);
  void AddCommandRemovedCallback(const CommandQueue::CommandCallback& callback);
  void AddCommandHandler(const std::string& command_name,
                         const CommandQueue::CommandCallback& callback);

  // Sets callback which is called when command definitions is changed.
  void AddCommandDefChanged(const base::Closure& callback);

  // Returns the command definitions for the device.
  const CommandDictionary& GetCommandDictionary() const;

  // Loads base/standard GCD command definitions.
  // |dict| is the full JSON schema of standard GCD commands. These commands
  // are not necessarily supported by a particular device but rather
  // all the standard commands defined by GCD standard for all known/supported
  // device kinds.
  // On success, returns true. Otherwise, |error| contains additional
  // error information.
  bool LoadBaseCommands(const base::DictionaryValue& dict, ErrorPtr* error);

  // Same as the overload above, but takes a path to a json file to read
  // the base command definitions from.
  bool LoadBaseCommands(const std::string& json, ErrorPtr* error);

  // Loads device command schema.
  bool LoadCommands(const base::DictionaryValue& dict,
                    ErrorPtr* error);

  // Same as the overload above, but takes a path to a json file to read
  // the base command definitions from.
  bool LoadCommands(const std::string& json,
                    ErrorPtr* error);

  // Startup method to be called by buffet daemon at startup.
  // Initializes standard GCD command dictionary.
  void Startup();

  // Adds a new command to the command queue.
  void AddCommand(std::unique_ptr<CommandInstance> command_instance);

  // Changes the visibility of commands.
  bool SetCommandVisibility(const std::vector<std::string>& command_names,
                            CommandDefinition::Visibility visibility,
                            ErrorPtr* error);

  bool AddCommand(const base::DictionaryValue& command,
                  UserRole role,
                  std::string* id,
                  ErrorPtr* error);

 private:
  CommandDictionary base_dictionary_;  // Base/std command definitions/schemas.
  CommandDictionary dictionary_;       // Command definitions/schemas.
  CommandQueue command_queue_;
  std::vector<base::Callback<void()>> on_command_changed_;
  uint32_t next_command_id_{0};

  DISALLOW_COPY_AND_ASSIGN(CommandManager);
};

}  // namespace weave

#endif  // LIBWEAVE_SRC_COMMANDS_COMMAND_MANAGER_H_
