// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/privet/auth_manager.h"

#include <base/rand_util.h>
#include <base/strings/string_number_conversions.h>

#include "src/data_encoding.h"
#include "src/privet/openssl_utils.h"
#include "src/string_utils.h"

extern "C" {
#include "third_party/libuweave/src/macaroon.h"
}

namespace weave {
namespace privet {

namespace {

const char kTokenDelimeter[] = ":";
const size_t kCaveatBuffetSize = 32;
const size_t kMaxMacaroonSize = 1024;

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
  if (time)
    *time = base::Time::FromTimeT(timestamp);
  return UserInfo{static_cast<AuthScope>(scope), id};
}

class Caveat {
 public:
  Caveat(UwMacaroonCaveatType type, uint32_t value) {
    CHECK(uw_macaroon_caveat_create_with_uint_(type, value, buffer,
                                               sizeof(buffer), &caveat));
  }

  const UwMacaroonCaveat& GetCaveat() const { return caveat; }

 private:
  UwMacaroonCaveat caveat;
  uint8_t buffer[kCaveatBuffetSize];

  DISALLOW_COPY_AND_ASSIGN(Caveat);
};

}  // namespace

AuthManager::AuthManager(const std::vector<uint8_t>& secret,
                         const std::vector<uint8_t>& certificate_fingerprint,
                         base::Clock* clock)
    : clock_{clock ? clock : &default_clock_},
      secret_{secret},
      certificate_fingerprint_{certificate_fingerprint} {
  if (secret_.size() != kSha256OutputSize) {
    secret_.resize(kSha256OutputSize);
    base::RandBytes(secret_.data(), secret_.size());
  }
}

AuthManager::~AuthManager() {}

// Returns "[hmac]scope:id:time".
std::vector<uint8_t> AuthManager::CreateAccessToken(const UserInfo& user_info) {
  std::string data_str{CreateTokenData(user_info, Now())};
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

std::vector<uint8_t> AuthManager::GetRootClientAuthToken() const {
  Caveat scope{kUwMacaroonCaveatTypeScope, kUwMacaroonCaveatScopeTypeOwner};
  Caveat issued{kUwMacaroonCaveatTypeIssued,
                static_cast<uint32_t>(Now().ToTimeT())};

  UwMacaroonCaveat caveats[] = {
      scope.GetCaveat(), issued.GetCaveat(),
  };

  UwMacaroon macaroon{};
  CHECK(uw_macaroon_new_from_root_key_(
      &macaroon, secret_.data(), secret_.size(), caveats, arraysize(caveats)));

  std::vector<uint8_t> token(kMaxMacaroonSize);
  size_t len = 0;
  CHECK(uw_macaroon_dump_(&macaroon, token.data(), token.size(), &len));
  token.resize(len);
  return token;
}

base::Time AuthManager::Now() const {
  return clock_->Now();
}

bool AuthManager::IsValidAuthToken(const std::vector<uint8_t>& token) const {
  std::vector<uint8_t> buffer(kMaxMacaroonSize);
  UwMacaroon macaroon{};
  if (!uw_macaroon_load_(token.data(), token.size(), buffer.data(),
                         buffer.size(), &macaroon)) {
    return false;
  }

  return uw_macaroon_verify_(&macaroon, secret_.data(), secret_.size());
}

}  // namespace privet
}  // namespace weave
