// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_STATES_STATE_CHANGE_QUEUE_H_
#define LIBWEAVE_SRC_STATES_STATE_CHANGE_QUEUE_H_

#include <map>
#include <vector>

#include <base/macros.h>

#include "src/states/state_change_queue_interface.h"

namespace weave {

// An object to record and retrieve device state change notification events.
class StateChangeQueue : public StateChangeQueueInterface {
 public:
  explicit StateChangeQueue(size_t max_queue_size);

  // Overrides from StateChangeQueueInterface.
  bool IsEmpty() const override { return state_changes_.empty(); }
  bool NotifyPropertiesUpdated(
      base::Time timestamp,
      std::unique_ptr<base::DictionaryValue> changed_properties) override;
  std::vector<StateChange> GetAndClearRecordedStateChanges() override;
  UpdateID GetLastStateChangeId() const override { return last_change_id_; }
  Token AddOnStateUpdatedCallback(
      const base::Callback<void(UpdateID)>& callback) override;
  void NotifyStateUpdatedOnServer(UpdateID update_id) override;

 private:
  // Maximum queue size. If it is full, the oldest state update records are
  // merged together until the queue size is within the size limit.
  const size_t max_queue_size_;

  // Accumulated list of device state change notifications.
  std::map<base::Time, std::unique_ptr<base::DictionaryValue>> state_changes_;

  // An ID of last state change update. Each NotifyPropertiesUpdated()
  // invocation increments this value by 1.
  UpdateID last_change_id_{0};

  // Callback list for state change queue even sinks.
  base::CallbackList<void(UpdateID)> callbacks_;

  DISALLOW_COPY_AND_ASSIGN(StateChangeQueue);
};

}  // namespace weave

#endif  // LIBWEAVE_SRC_STATES_STATE_CHANGE_QUEUE_H_
