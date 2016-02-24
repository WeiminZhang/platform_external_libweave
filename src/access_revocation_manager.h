// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_ACCESS_REVOCATION_MANAGER_H_
#define LIBWEAVE_SRC_ACCESS_REVOCATION_MANAGER_H_

#include <vector>

#include <base/time/time.h>

namespace weave {

class AccessRevocationManager {
 public:
  struct Entry {
    Entry() = default;

    Entry(const std::vector<uint8_t>& user,
          const std::vector<uint8_t>& app,
          base::Time revocation_ts,
          base::Time expiration_ts)
        : user_id{user},
          app_id{app},
          revocation{revocation_ts},
          expiration{expiration_ts} {}
    // user_id is empty, app_id is empty: block everything.
    // user_id is not empty, app_id is empty: block if user_id matches.
    // user_id is empty, app_id is not empty: block if app_id matches.
    // user_id is not empty, app_id is not empty: block if both match.
    std::vector<uint8_t> user_id;
    std::vector<uint8_t> app_id;

    // Revoke matching entries if |revocation| is not less than
    // delegation timestamp.
    base::Time revocation;

    // Time after which to discard the rule.
    base::Time expiration;
  };
  virtual ~AccessRevocationManager() = default;

  virtual void AddEntryAddedCallback(const base::Closure& callback) = 0;
  virtual void Block(const Entry& entry, const DoneCallback& callback) = 0;
  virtual bool IsBlocked(const std::vector<uint8_t>& user_id,
                         const std::vector<uint8_t>& app_id,
                         base::Time timestamp) const = 0;
  virtual std::vector<Entry> GetEntries() const = 0;
  virtual size_t GetSize() const = 0;
  virtual size_t GetCapacity() const = 0;
};

inline bool operator==(const AccessRevocationManager::Entry& l,
                       const AccessRevocationManager::Entry& r) {
  return l.revocation == r.revocation && l.expiration == r.expiration &&
         l.user_id == r.user_id && l.app_id == r.app_id;
}

inline bool operator!=(const AccessRevocationManager::Entry& l,
                       const AccessRevocationManager::Entry& r) {
  return !(l == r);
}

}  // namespace weave

#endif  // LIBWEAVE_SRC_ACCESS_REVOCATION_MANAGER_H_
