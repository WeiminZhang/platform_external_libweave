// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_ACCESS_BLACK_LIST_H_
#define LIBWEAVE_SRC_ACCESS_BLACK_LIST_H_

#include <vector>

#include <base/time/time.h>

namespace weave {

class AccessBlackListManager {
 public:
  struct Entry {
    std::vector<uint8_t> user_id;
    std::vector<uint8_t> app_id;
    base::Time expiration;
  };
  virtual ~AccessBlackListManager() = default;

  virtual void Block(const std::vector<uint8_t>& user_id,
                     const std::vector<uint8_t>& app_id,
                     const base::Time& expiration,
                     const DoneCallback& callback) = 0;
  virtual bool Unblock(const std::vector<uint8_t>& user_id,
                       const std::vector<uint8_t>& app_id,
                       ErrorPtr* error) = 0;
  virtual std::vector<Entry> GetEntries() const = 0;
  virtual size_t GetSize() const = 0;
  virtual size_t GetCapacity() const = 0;
};

}  // namespace weave

#endif  // LIBWEAVE_SRC_ACCESS_BLACK_LIST_H_
