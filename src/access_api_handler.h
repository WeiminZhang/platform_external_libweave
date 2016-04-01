// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_ACCESS_API_HANDLER_H_
#define LIBWEAVE_SRC_ACCESS_API_HANDLER_H_

#include <memory>

#include <base/memory/weak_ptr.h>
#include <weave/error.h>

namespace weave {

class AccessRevocationManager;
class Command;
class Device;

// Handles commands for 'blacklist' trait.
// Objects of the class subscribe for notification from CommandManager and
// execute incoming commands.
// Handled commands:
//  blacklist.add
//  blacklist.list
class AccessApiHandler final {
 public:
  AccessApiHandler(Device* device, AccessRevocationManager* manager);

 private:
  void Block(const std::weak_ptr<Command>& command);
  void List(const std::weak_ptr<Command>& command);
  void UpdateState();

  void OnCommandDone(const std::weak_ptr<Command>& command, ErrorPtr error);

  Device* device_{nullptr};
  AccessRevocationManager* manager_{nullptr};

  base::WeakPtrFactory<AccessApiHandler> weak_ptr_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(AccessApiHandler);
};

}  // namespace weave

#endif  // LIBWEAVE_SRC_ACCESS_API_HANDLER_H_
