// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/privet/auth_manager.h"

#include <algorithm>

#include <base/guid.h>
#include <base/rand_util.h>
#include <base/strings/string_number_conversions.h>

#include "src/config.h"
#include "src/data_encoding.h"
#include "src/privet/constants.h"
#include "src/privet/openssl_utils.h"
#include "src/string_utils.h"

extern "C" {
#include "third_party/libuweave/src/macaroon.h"
}

namespace weave {
namespace privet {

namespace {

const char kTokenDelimeter[] = ":";
const size_t kMaxMacaroonSize = 1024;
const size_t kMaxPendingClaims = 10;
const char kInvalidTokenError[] = "invalid_token";

template <class T>
void AppendToArray(T value, std::vector<uint8_t>* array) {
  auto begin = reinterpret_cast<const uint8_t*>(&value);
  array->insert(array->end(), begin, begin + sizeof(value));
}

// Returns "scope:time:id".
std::string CreateTokenData(const UserInfo& user_info, const base::Time& time) {
  return base::IntToString(static_cast<int>(user_info.scope())) +
         kTokenDelimeter + std::to_string(time.ToTimeT()) + kTokenDelimeter +
         user_info.user_id();
}

// Splits string of "scope:time:id" format.
UserInfo SplitTokenData(const std::string& token, base::Time* time) {
  const UserInfo kNone;
  auto parts = SplitAtFirst(token, kTokenDelimeter, false);
  if (parts.second.empty())
    return kNone;
  int scope = 0;
  if (!base::StringToInt(parts.first, &scope) ||
      scope < static_cast<int>(AuthScope::kNone) ||
      scope > static_cast<int>(AuthScope::kOwner)) {
    return kNone;
  }

  parts = SplitAtFirst(parts.second, kTokenDelimeter, false);
  int64_t timestamp{0};
  if (parts.second.empty() || !base::StringToInt64(parts.first, &timestamp))
    return kNone;

  if (time)
    *time = base::Time::FromTimeT(timestamp);
  return UserInfo{static_cast<AuthScope>(scope), parts.second};
}

class Caveat {
 public:
  // TODO(vitalybuka): Use _get_buffer_size_ when available.
  Caveat(UwMacaroonCaveatType type, uint32_t value) : buffer(8) {
    CHECK(uw_macaroon_caveat_create_with_uint_(type, value, buffer.data(),
                                               buffer.size(), &caveat));
  }

  // TODO(vitalybuka): Use _get_buffer_size_ when available.
  Caveat(UwMacaroonCaveatType type, const std::string& value)
      : buffer(std::max<size_t>(value.size(), 32u) * 2) {
    CHECK(uw_macaroon_caveat_create_with_str_(
        type, reinterpret_cast<const uint8_t*>(value.data()), value.size(),
        buffer.data(), buffer.size(), &caveat));
  }

  const UwMacaroonCaveat& GetCaveat() const { return caveat; }

 private:
  UwMacaroonCaveat caveat;
  std::vector<uint8_t> buffer;

  DISALLOW_COPY_AND_ASSIGN(Caveat);
};

std::vector<uint8_t> CreateSecret() {
  std::vector<uint8_t> secret(kSha256OutputSize);
  base::RandBytes(secret.data(), secret.size());
  return secret;
}

bool IsClaimAllowed(RootClientTokenOwner curret, RootClientTokenOwner claimer) {
  return claimer > curret || claimer == RootClientTokenOwner::kCloud;
}

std::vector<uint8_t> CreateMacaroonToken(
    const std::vector<uint8_t>& secret,
    const std::vector<UwMacaroonCaveat>& caveats) {
  CHECK_EQ(kSha256OutputSize, secret.size());
  UwMacaroon macaroon{};
  CHECK(uw_macaroon_new_from_root_key_(&macaroon, secret.data(), secret.size(),
                                       caveats.data(), caveats.size()));

  std::vector<uint8_t> token(kMaxMacaroonSize);
  size_t len = 0;
  CHECK(uw_macaroon_dump_(&macaroon, token.data(), token.size(), &len));
  token.resize(len);

  return token;
}

bool LoadMacaroon(const std::vector<uint8_t>& token,
                  std::vector<uint8_t>* buffer,
                  UwMacaroon* macaroon,
                  ErrorPtr* error) {
  buffer->resize(kMaxMacaroonSize);
  if (!uw_macaroon_load_(token.data(), token.size(), buffer->data(),
                         buffer->size(), macaroon)) {
    Error::AddTo(error, FROM_HERE, errors::kDomain, kInvalidTokenError,
                 "Invalid token format");
    return false;
  }
  return true;
}

bool VerifyMacaroon(const std::vector<uint8_t>& secret,
                    const UwMacaroon& macaroon,
                    ErrorPtr* error) {
  CHECK_EQ(kSha256OutputSize, secret.size());
  if (!uw_macaroon_verify_(&macaroon, secret.data(), secret.size())) {
    Error::AddTo(error, FROM_HERE, errors::kDomain, "invalid_signature",
                 "Invalid token signature");
    return false;
  }
  return true;
}

}  // namespace

AuthManager::AuthManager(Config* config,
                         const std::vector<uint8_t>& certificate_fingerprint)
    : config_{config},
      certificate_fingerprint_{certificate_fingerprint},
      access_secret_{CreateSecret()} {
  if (config_) {
    SetAuthSecret(config_->GetSettings().secret,
                  config_->GetSettings().root_client_token_owner);
  } else {
    SetAuthSecret({}, RootClientTokenOwner::kNone);
  }
}

AuthManager::AuthManager(const std::vector<uint8_t>& auth_secret,
                         const std::vector<uint8_t>& certificate_fingerprint,
                         const std::vector<uint8_t>& access_secret,
                         base::Clock* clock)
    : AuthManager(nullptr, certificate_fingerprint) {
  access_secret_ = access_secret.size() == kSha256OutputSize ? access_secret
                                                             : CreateSecret();
  SetAuthSecret(auth_secret, RootClientTokenOwner::kNone);
  if (clock)
    clock_ = clock;
}

void AuthManager::SetAuthSecret(const std::vector<uint8_t>& secret,
                                RootClientTokenOwner owner) {
  auth_secret_ = secret;

  if (auth_secret_.size() != kSha256OutputSize) {
    auth_secret_ = CreateSecret();
    owner = RootClientTokenOwner::kNone;
  }

  if (!config_ || (config_->GetSettings().secret == auth_secret_ &&
                   config_->GetSettings().root_client_token_owner == owner)) {
    return;
  }

  Config::Transaction change{config_};
  change.set_secret(secret);
  change.set_root_client_token_owner(owner);
  change.Commit();
}

AuthManager::~AuthManager() {}

// Returns "[hmac]scope:expiration_time:id".
std::vector<uint8_t> AuthManager::CreateAccessToken(const UserInfo& user_info,
                                                    base::TimeDelta ttl) const {
  std::string data_str{CreateTokenData(user_info, Now() + ttl)};
  std::vector<uint8_t> data{data_str.begin(), data_str.end()};
  std::vector<uint8_t> hash{HmacSha256(access_secret_, data)};
  hash.insert(hash.end(), data.begin(), data.end());

  return hash;
}

// TODO(vitalybuka): Switch to Macaroon?
// Parses "base64([hmac]scope:id:expriration_time)".
bool AuthManager::ParseAccessToken(const std::vector<uint8_t>& token,
                                   UserInfo* user_info,
                                   ErrorPtr* error) const {
  if (token.size() <= kSha256OutputSize) {
    Error::AddToPrintf(error, FROM_HERE, errors::kDomain,
                       errors::kInvalidAuthorization, "Invalid token size: %zu",
                       token.size());
    return false;
  }
  std::vector<uint8_t> hash(token.begin(), token.begin() + kSha256OutputSize);
  std::vector<uint8_t> data(token.begin() + kSha256OutputSize, token.end());
  if (hash != HmacSha256(access_secret_, data)) {
    Error::AddTo(error, FROM_HERE, errors::kDomain,
                 errors::kInvalidAuthorization, "Invalid signature");
    return false;
  }

  base::Time time;
  UserInfo info = SplitTokenData(std::string(data.begin(), data.end()), &time);
  if (info.scope() == AuthScope::kNone) {
    Error::AddTo(error, FROM_HERE, errors::kDomain,
                 errors::kInvalidAuthorization, "Invalid token data");
    return false;
  }

  if (time < clock_->Now()) {
    Error::AddTo(error, FROM_HERE, errors::kDomain,
                 errors::kAuthorizationExpired, "Token is expired");
    return false;
  }

  if (user_info)
    *user_info = info;

  return true;
}

std::vector<uint8_t> AuthManager::ClaimRootClientAuthToken(
    RootClientTokenOwner owner,
    ErrorPtr* error) {
  CHECK(RootClientTokenOwner::kNone != owner);
  if (config_) {
    auto current = config_->GetSettings().root_client_token_owner;
    if (!IsClaimAllowed(current, owner)) {
      Error::AddToPrintf(
          error, FROM_HERE, errors::kDomain, errors::kAlreadyClaimed,
          "Device already claimed by '%s'", EnumToString(current).c_str());
      return {};
    }
  };

  pending_claims_.push_back(std::make_pair(
      std::unique_ptr<AuthManager>{new AuthManager{nullptr, {}}}, owner));
  if (pending_claims_.size() > kMaxPendingClaims)
    pending_claims_.pop_front();
  return pending_claims_.back().first->GetRootClientAuthToken();
}

bool AuthManager::ConfirmClientAuthToken(const std::vector<uint8_t>& token,
                                         ErrorPtr* error) {
  // Cover case when caller sent confirm twice.
  if (pending_claims_.empty())
    return IsValidAuthToken(token, error);

  auto claim =
      std::find_if(pending_claims_.begin(), pending_claims_.end(),
                   [&token](const decltype(pending_claims_)::value_type& auth) {
                     return auth.first->IsValidAuthToken(token, nullptr);
                   });
  if (claim == pending_claims_.end()) {
    Error::AddTo(error, FROM_HERE, errors::kDomain, errors::kNotFound,
                 "Unknown claim");
    return false;
  }

  SetAuthSecret(claim->first->GetAuthSecret(), claim->second);
  pending_claims_.clear();
  return true;
}

std::vector<uint8_t> AuthManager::GetRootClientAuthToken() const {
  Caveat scope{kUwMacaroonCaveatTypeScope, kUwMacaroonCaveatScopeTypeOwner};
  Caveat issued{kUwMacaroonCaveatTypeIssued,
                static_cast<uint32_t>(Now().ToTimeT())};
  return CreateMacaroonToken(auth_secret_,
                             {
                                 scope.GetCaveat(), issued.GetCaveat(),
                             });
}

base::Time AuthManager::Now() const {
  return clock_->Now();
}

bool AuthManager::IsValidAuthToken(const std::vector<uint8_t>& token,
                                   ErrorPtr* error) const {
  std::vector<uint8_t> buffer;
  UwMacaroon macaroon{};
  if (!LoadMacaroon(token, &buffer, &macaroon, error) ||
      !VerifyMacaroon(auth_secret_, macaroon, error)) {
    Error::AddTo(error, FROM_HERE, errors::kDomain, errors::kInvalidAuthCode,
                 "Invalid token");
    return false;
  }
  return true;
}

bool AuthManager::CreateAccessTokenFromAuth(
    const std::vector<uint8_t>& auth_token,
    base::TimeDelta ttl,
    std::vector<uint8_t>* access_token,
    AuthScope* access_token_scope,
    base::TimeDelta* access_token_ttl,
    ErrorPtr* error) const {
  // TODO(vitalybuka): implement token validation.
  if (!IsValidAuthToken(auth_token, error))
    return false;

  if (!access_token)
    return true;

  // TODO(vitalybuka): User and scope must be parsed from auth_token.
  UserInfo info{config_ ? config_->GetSettings().local_anonymous_access_role
                        : AuthScope::kViewer,
                base::GenerateGUID()};

  // TODO(vitalybuka): TTL also should be reduced in accordance with auth_token.
  *access_token = CreateAccessToken(info, ttl);

  if (access_token_scope)
    *access_token_scope = info.scope();

  if (access_token_ttl)
    *access_token_ttl = ttl;
  return true;
}

std::vector<uint8_t> AuthManager::CreateSessionId() {
  std::vector<uint8_t> result;
  AppendToArray(Now().ToTimeT(), &result);
  AppendToArray(++session_counter_, &result);
  return result;
}

}  // namespace privet
}  // namespace weave
