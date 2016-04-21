// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/access_revocation_manager_impl.h"

#include <base/json/json_reader.h>
#include <base/json/json_writer.h>
#include <base/values.h>

#include "src/commands/schema_constants.h"
#include "src/data_encoding.h"
#include "src/utils.h"

namespace weave {

namespace {
const char kConfigFileName[] = "black_list";

const char kUser[] = "user";
const char kApp[] = "app";
const char kExpiration[] = "expiration";
const char kRevocation[] = "revocation";
}

AccessRevocationManagerImpl::AccessRevocationManagerImpl(
    provider::ConfigStore* store,
    size_t capacity,
    base::Clock* clock)
    : capacity_{capacity}, clock_{clock}, store_{store} {
  Load();
}

void AccessRevocationManagerImpl::Load() {
  if (!store_)
    return;
  if (auto list = base::ListValue::From(
          base::JSONReader::Read(store_->LoadSettings(kConfigFileName)))) {
    for (const auto& value : *list) {
      const base::DictionaryValue* entry{nullptr};
      std::string user;
      std::string app;
      Entry e;
      int revocation = 0;
      int expiration = 0;
      if (value->GetAsDictionary(&entry) && entry->GetString(kUser, &user) &&
          Base64Decode(user, &e.user_id) && entry->GetString(kApp, &app) &&
          Base64Decode(app, &e.app_id) &&
          entry->GetInteger(kRevocation, &revocation) &&
          entry->GetInteger(kExpiration, &expiration)) {
        e.revocation = FromJ2000Time(revocation);
        e.expiration = FromJ2000Time(expiration);
        if (e.expiration > clock_->Now())
          entries_.insert(e);
      }
    }
    if (entries_.size() < list->GetSize()) {
      // Save some storage space by saving without expired entries.
      Save({});
    }
  }
}

void AccessRevocationManagerImpl::Save(const DoneCallback& callback) {
  if (!store_) {
    if (!callback.is_null())
      callback.Run(nullptr);
    return;
  }

  base::ListValue list;
  for (const auto& e : entries_) {
    scoped_ptr<base::DictionaryValue> entry{new base::DictionaryValue};
    entry->SetString(kUser, Base64Encode(e.user_id));
    entry->SetString(kApp, Base64Encode(e.app_id));
    entry->SetInteger(kRevocation, ToJ2000Time(e.revocation));
    entry->SetInteger(kExpiration, ToJ2000Time(e.expiration));
    list.Append(std::move(entry));
  }

  std::string json;
  base::JSONWriter::Write(list, &json);
  store_->SaveSettings(kConfigFileName, json, callback);
}

void AccessRevocationManagerImpl::Shrink() {
  base::Time oldest[2] = {base::Time::Max(), base::Time::Max()};
  for (auto i = begin(entries_); i != end(entries_);) {
    if (i->expiration <= clock_->Now())
      i = entries_.erase(i);
    else {
      // Non-strict comparison to ensure counting same timestamps as different.
      if (i->revocation <= oldest[0]) {
        oldest[1] = oldest[0];
        oldest[0] = i->revocation;
      } else {
        oldest[1] = std::min(oldest[1], i->revocation);
      }
      ++i;
    }
  }
  CHECK_GT(capacity_, 1u);
  if (entries_.size() >= capacity_) {
    // List is full so we are going to remove oldest entries from the list.
    for (auto i = begin(entries_); i != end(entries_);) {
      if (i->revocation <= oldest[1])
        i = entries_.erase(i);
      else {
        ++i;
      }
    }
    // And replace with a single rule to block everything older.
    Entry all_blocking_entry;
    all_blocking_entry.expiration = base::Time::Max();
    all_blocking_entry.revocation = oldest[1];
    entries_.insert(all_blocking_entry);
  }
}

void AccessRevocationManagerImpl::AddEntryAddedCallback(
    const base::Closure& callback) {
  on_entry_added_callbacks_.push_back(callback);
}

void AccessRevocationManagerImpl::Block(const Entry& entry,
                                        const DoneCallback& callback) {
  if (entry.expiration <= clock_->Now()) {
    if (!callback.is_null()) {
      ErrorPtr error;
      Error::AddTo(&error, FROM_HERE, "aleady_expired",
                   "Entry already expired");
      callback.Run(std::move(error));
    }
    return;
  }

  // Iterating is OK as Save below is more expensive.
  Shrink();
  CHECK_LT(entries_.size(), capacity_);

  auto existing = entries_.find(entry);
  if (existing != entries_.end()) {
    Entry new_entry = entry;
    new_entry.expiration = std::max(entry.expiration, existing->expiration);
    new_entry.revocation = std::max(entry.revocation, existing->revocation);
    entries_.erase(existing);
    entries_.insert(new_entry);
  } else {
    entries_.insert(entry);
  }

  for (const auto& cb : on_entry_added_callbacks_)
    cb.Run();

  Save(callback);
}

bool AccessRevocationManagerImpl::IsBlocked(const std::vector<uint8_t>& user_id,
                                            const std::vector<uint8_t>& app_id,
                                            base::Time timestamp) const {
  Entry entry_to_find;
  const std::vector<uint8_t> no_id;
  for (const auto& user : {no_id, user_id}) {
    for (const auto& app : {no_id, app_id}) {
      entry_to_find.user_id = user;
      entry_to_find.app_id = app;
      auto match = entries_.find(entry_to_find);
      if (match != end(entries_) && match->expiration > clock_->Now() &&
          match->revocation >= timestamp) {
        return true;
      }
    }
  }
  return false;
}

std::vector<AccessRevocationManager::Entry>
AccessRevocationManagerImpl::GetEntries() const {
  return {begin(entries_), end(entries_)};
}

size_t AccessRevocationManagerImpl::GetSize() const {
  return entries_.size();
}

size_t AccessRevocationManagerImpl::GetCapacity() const {
  return capacity_;
}

}  // namespace weave
