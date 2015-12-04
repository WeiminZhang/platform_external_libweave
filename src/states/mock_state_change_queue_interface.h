// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_STATES_MOCK_STATE_CHANGE_QUEUE_INTERFACE_H_
#define LIBWEAVE_SRC_STATES_MOCK_STATE_CHANGE_QUEUE_INTERFACE_H_

#include <vector>

#include <gmock/gmock.h>

#include "src/states/state_change_queue_interface.h"

namespace weave {

class MockStateChangeQueueInterface : public StateChangeQueueInterface {
 public:
  MOCK_CONST_METHOD0(IsEmpty, bool());
  MOCK_METHOD2(NotifyPropertiesUpdated,
               bool(base::Time timestamp,
                    const base::DictionaryValue& changed_properties));
  MOCK_METHOD0(MockGetAndClearRecordedStateChanges,
               std::vector<StateChange>&());
  MOCK_CONST_METHOD0(GetLastStateChangeId, UpdateID());
  MOCK_METHOD1(MockAddOnStateUpdatedCallback,
               base::CallbackList<void(UpdateID)>::Subscription*(
                   const base::Callback<void(UpdateID)>&));
  MOCK_METHOD1(NotifyStateUpdatedOnServer, void(UpdateID));

 private:
  std::vector<StateChange> GetAndClearRecordedStateChanges() override {
    return std::move(MockGetAndClearRecordedStateChanges());
  }

  Token AddOnStateUpdatedCallback(
      const base::Callback<void(UpdateID)>& callback) override {
    return Token{MockAddOnStateUpdatedCallback(callback)};
  }
};

}  // namespace weave

#endif  // LIBWEAVE_SRC_STATES_MOCK_STATE_CHANGE_QUEUE_INTERFACE_H_
