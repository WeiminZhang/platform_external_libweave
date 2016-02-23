// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_MOCK_BLACK_LIST_MANAGER_H_
#define LIBWEAVE_SRC_MOCK_BLACK_LIST_MANAGER_H_

#include <gmock/gmock.h>

#include "src/access_black_list_manager.h"

namespace weave {

namespace test {

class MockAccessBlackListManager : public AccessBlackListManager {
 public:
  MOCK_METHOD1(AddEntryAddedCallback, void(const base::Closure&));
  MOCK_METHOD4(Block,
               void(const std::vector<uint8_t>&,
                    const std::vector<uint8_t>&,
                    const base::Time&,
                    const DoneCallback&));
  MOCK_CONST_METHOD2(IsBlocked,
                     bool(const std::vector<uint8_t>&,
                          const std::vector<uint8_t>&));
  MOCK_CONST_METHOD0(GetEntries, std::vector<Entry>());
  MOCK_CONST_METHOD0(GetSize, size_t());
  MOCK_CONST_METHOD0(GetCapacity, size_t());
};

}  // namespace test

}  // namespace weave

#endif  // LIBWEAVE_SRC_MOCK_BLACK_LIST_MANAGER_H_
