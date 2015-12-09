// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/privet/auth_manager.h"

#include <base/rand_util.h>
#include <base/strings/string_number_conversions.h>

#include "src/data_encoding.h"
#include "src/privet/openssl_utils.h"
#include "src/string_utils.h"

namespace weave {
namespace privet {

namespace {

const char kTokenDelimeter[] = ":";

// Returns "scope:id:time".
std::string CreateTokenData(const UserInfo& user_info, const base::Time& time) {
  return base::IntToString(static_cast<int>(user_info.scope())) +
         kTokenDelimeter + base::Uint64ToString(user_info.user_id()) +
         kTokenDelimeter + base::Int64ToString(time.ToTimeT());
}

// Splits string of "scope:id:time" format.
UserInfo SplitTokenData(const std::string& token, base::Time* time) {
  const UserInfo kNone;
  auto parts = Split(token, kTokenDelimeter, false, false);
  if (parts.size() != 3)
    return kNone;
  int scope = 0;
  if (!base::StringToInt(parts[0], &scope) ||
      scope < static_cast<int>(AuthScope::kNone) ||
      scope > static_cast<int>(AuthScope::kOwner)) {
    return kNone;
  }

  uint64_t id{0};
  if (!base::StringToUint64(parts[1], &id))
    return kNone;

  int64_t timestamp{0};
  if (!base::StringToInt64(parts[2], &timestamp))
    return kNone;
  *time = base::Time::FromTimeT(timestamp);
  return UserInfo{static_cast<AuthScope>(scope), id};
}

}  // namespace

AuthManager::AuthManager(const std::vector<uint8_t>& secret,
                         const std::vector<uint8_t>& certificate_fingerprint)
    : secret_{secret}, certificate_fingerprint_{certificate_fingerprint} {
  if (secret_.size() != kSha256OutputSize) {
    secret_.resize(kSha256OutputSize);
    base::RandBytes(secret_.data(), secret_.size());
  }
}

AuthManager::~AuthManager() {}

// Returns "[hmac]scope:id:time".
std::vector<uint8_t> AuthManager::CreateAccessToken(const UserInfo& user_info,
                                                    const base::Time& time) {
  std::string data_str{CreateTokenData(user_info, time)};
  std::vector<uint8_t> data{data_str.begin(), data_str.end()};
  std::vector<uint8_t> hash{HmacSha256(secret_, data)};
  hash.insert(hash.end(), data.begin(), data.end());
  return hash;
}

// Parses "base64([hmac]scope:id:time)".
UserInfo AuthManager::ParseAccessToken(const std::vector<uint8_t>& token,
                                       base::Time* time) const {
  if (token.size() <= kSha256OutputSize)
    return UserInfo{};
  std::vector<uint8_t> hash(token.begin(), token.begin() + kSha256OutputSize);
  std::vector<uint8_t> data(token.begin() + kSha256OutputSize, token.end());
  if (hash != HmacSha256(secret_, data))
    return UserInfo{};
  return SplitTokenData(std::string(data.begin(), data.end()), time);
}

}  // namespace privet
}  // namespace weave
