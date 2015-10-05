// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_COMMANDS_H_
#define LIBWEAVE_INCLUDE_WEAVE_COMMANDS_H_

#include <string>

#include <base/callback.h>
#include <base/values.h>
#include <weave/error.h>
#include <weave/command.h>

namespace weave {

class Commands {
 public:
  using CommandCallback = base::Callback<void(Command*)>;

  // Adds notification callback for a new command being added to the queue.
  virtual void AddCommandAddedCallback(const CommandCallback& callback) = 0;

  // Adds notification callback for a command being removed from the queue.
  virtual void AddCommandRemovedCallback(const CommandCallback& callback) = 0;

  // Adds a new command to the command queue.
  virtual bool AddCommand(const base::DictionaryValue& command,
                          std::string* id,
                          ErrorPtr* error) = 0;

  // Finds a command by the command |id|. Returns nullptr if the command with
  // the given |id| is not found. The returned pointer should not be persisted
  // for a long period of time.
  virtual Command* FindCommand(const std::string& id) = 0;

 protected:
  virtual ~Commands() = default;
};

}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_COMMANDS_H_
