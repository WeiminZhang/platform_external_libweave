// Copyright 2015 The Weave Authors. All rights reserved.
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
                         const Device::CommandHandlerCallback& callback);

  // Sets callback which is called when command definitions is changed.
  void AddCommandDefChanged(const base::Closure& callback);

  // Returns the command definitions for the device.
  const CommandDictionary& GetCommandDictionary() const;

  // Loads device command schema.
  bool LoadCommands(const base::DictionaryValue& dict,
                    ErrorPtr* error);

  // Same as the overload above, but takes a path to a json file to read
  // the base command definitions from.
  bool LoadCommands(const std::string& json,
                    ErrorPtr* error);

  // Adds a new command to the command queue.
  void AddCommand(std::unique_ptr<CommandInstance> command_instance);

  bool AddCommand(const base::DictionaryValue& command,
                  UserRole role,
                  std::string* id,
                  ErrorPtr* error);

 private:
  CommandDictionary dictionary_;  // Registered definitions.
  CommandQueue command_queue_;
  std::vector<base::Callback<void()>> on_command_changed_;
  uint32_t next_command_id_{0};

  DISALLOW_COPY_AND_ASSIGN(CommandManager);
};

}  // namespace weave

#endif  // LIBWEAVE_SRC_COMMANDS_COMMAND_MANAGER_H_
