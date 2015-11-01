// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/commands/command_queue.h"

#include <base/bind.h>
#include <base/time/time.h>

namespace weave {

namespace {
const int kRemoveCommandDelayMin = 5;
}

void CommandQueue::AddCommandAddedCallback(const CommandCallback& callback) {
  on_command_added_.push_back(callback);
  // Send all pre-existed commands.
  for (const auto& command : map_)
    callback.Run(command.second.get());
}

void CommandQueue::AddCommandRemovedCallback(const CommandCallback& callback) {
  on_command_removed_.push_back(callback);
}

void CommandQueue::AddCommandHandler(
    const std::string& command_name,
    const Device::CommandHandlerCallback& callback) {
  if (!command_name.empty()) {
    CHECK(default_command_callback_.is_null())
        << "Commands specific handler are not allowed after default one";

    for (const auto& command : map_) {
      if (command.second->GetState() == Command::State::kQueued &&
          command.second->GetName() == command_name) {
        callback.Run(command.second);
      }
    }

    CHECK(command_callbacks_.emplace(command_name, callback).second)
        << command_name << " already has handler";

  } else {
    for (const auto& command : map_) {
      if (command.second->GetState() == Command::State::kQueued &&
          command_callbacks_.find(command.second->GetName()) ==
              command_callbacks_.end()) {
        callback.Run(command.second);
      }
    }

    CHECK(default_command_callback_.is_null()) << "Already has default handler";
    default_command_callback_ = callback;
  }
}

void CommandQueue::Add(std::unique_ptr<CommandInstance> instance) {
  std::string id = instance->GetID();
  LOG_IF(FATAL, id.empty()) << "Command has no ID";
  instance->AttachToQueue(this);
  auto pair = map_.insert(std::make_pair(id, std::move(instance)));
  LOG_IF(FATAL, !pair.second) << "Command with ID '" << id
                              << "' is already in the queue";
  for (const auto& cb : on_command_added_)
    cb.Run(pair.first->second.get());

  auto it_handler = command_callbacks_.find(pair.first->second->GetName());

  if (it_handler != command_callbacks_.end())
    it_handler->second.Run(pair.first->second);
  else if (!default_command_callback_.is_null())
    default_command_callback_.Run(pair.first->second);

  Cleanup();
}

void CommandQueue::DelayedRemove(const std::string& id) {
  auto p = map_.find(id);
  if (p == map_.end())
    return;
  remove_queue_.push(std::make_pair(
      base::Time::Now() + base::TimeDelta::FromMinutes(kRemoveCommandDelayMin),
      id));
  Cleanup();
}

bool CommandQueue::Remove(const std::string& id) {
  auto p = map_.find(id);
  if (p == map_.end())
    return false;
  std::shared_ptr<CommandInstance> instance = p->second;
  instance->DetachFromQueue();
  map_.erase(p);
  for (const auto& cb : on_command_removed_)
    cb.Run(instance.get());
  return true;
}

void CommandQueue::Cleanup() {
  while (!remove_queue_.empty() && remove_queue_.front().first < Now()) {
    Remove(remove_queue_.front().second);
    remove_queue_.pop();
  }
}

void CommandQueue::SetNowForTest(base::Time now) {
  test_now_ = now;
}

base::Time CommandQueue::Now() const {
  return test_now_.is_null() ? base::Time::Now() : test_now_;
}

CommandInstance* CommandQueue::Find(const std::string& id) const {
  auto p = map_.find(id);
  return (p != map_.end()) ? p->second.get() : nullptr;
}

}  // namespace weave
