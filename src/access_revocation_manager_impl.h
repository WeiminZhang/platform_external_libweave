// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_ACCESS_REVOCATION_MANAGER_IMPL_H_
#define LIBWEAVE_SRC_ACCESS_REVOCATION_MANAGER_IMPL_H_

#include <set>
#include <utility>

#include <base/time/default_clock.h>
#include <base/time/time.h>
#include <weave/error.h>
#include <weave/provider/config_store.h>

#include "src/access_revocation_manager.h"

namespace weave {

class AccessRevocationManagerImpl : public AccessRevocationManager {
 public:
  explicit AccessRevocationManagerImpl(provider::ConfigStore* store,
                                       size_t capacity = 1024,
                                       base::Clock* clock = nullptr);

  // AccessRevocationManager implementation.
  void AddEntryAddedCallback(const base::Closure& callback) override;
  void Block(const Entry& entry, const DoneCallback& callback) override;
  bool IsBlocked(const std::vector<uint8_t>& user_id,
                 const std::vector<uint8_t>& app_id,
                 base::Time timestamp) const override;
  std::vector<Entry> GetEntries() const override;
  size_t GetSize() const override;
  size_t GetCapacity() const override;

 private:
  void Load();
  void Save(const DoneCallback& callback);
  void Shrink();

  struct EntryIdsLess {
    bool operator()(const Entry& l, const Entry& r) const {
      auto make_tuple = [](const AccessRevocationManager::Entry& e) {
        return std::tie(e.user_id, e.app_id);
      };
      return make_tuple(l) < make_tuple(r);
    }
  };

  const size_t capacity_{0};
  base::DefaultClock default_clock_;
  base::Clock* clock_{&default_clock_};

  provider::ConfigStore* store_{nullptr};
  std::set<Entry, EntryIdsLess> entries_;
  std::vector<base::Closure> on_entry_added_callbacks_;

  DISALLOW_COPY_AND_ASSIGN(AccessRevocationManagerImpl);
};

}  // namespace weave

#endif  // LIBWEAVE_SRC_ACCESS_REVOCATION_MANAGER_IMPL_H_
