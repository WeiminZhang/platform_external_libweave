// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/privet/security_manager.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <set>

#include <base/bind.h>
#include <base/guid.h>
#include <base/logging.h>
#include <base/rand_util.h>
#include <base/strings/string_number_conversions.h>
#include <base/strings/stringprintf.h>
#include <base/time/time.h>
#include <weave/provider/task_runner.h>

#include "src/config.h"
#include "src/data_encoding.h"
#include "src/privet/auth_manager.h"
#include "src/privet/constants.h"
#include "src/privet/openssl_utils.h"
#include "src/string_utils.h"
#include "third_party/chromium/crypto/p224_spake.h"

namespace weave {
namespace privet {

namespace {

const int kSessionExpirationTimeMinutes = 5;
const int kPairingExpirationTimeMinutes = 5;
const int kMaxAllowedPairingAttemts = 3;
const int kPairingBlockingTimeMinutes = 1;

const int kAccessTokenExpirationSeconds = 3600;

class Spakep224Exchanger : public SecurityManager::KeyExchanger {
 public:
  explicit Spakep224Exchanger(const std::string& password)
      : spake_(crypto::P224EncryptedKeyExchange::kPeerTypeServer, password) {}
  ~Spakep224Exchanger() override = default;

  // SecurityManager::KeyExchanger methods.
  const std::string& GetMessage() override { return spake_.GetNextMessage(); }

  bool ProcessMessage(const std::string& message, ErrorPtr* error) override {
    switch (spake_.ProcessMessage(message)) {
      case crypto::P224EncryptedKeyExchange::kResultPending:
        return true;
      case crypto::P224EncryptedKeyExchange::kResultFailed:
        Error::AddTo(error, FROM_HERE, errors::kDomain,
                     errors::kInvalidClientCommitment, spake_.error());
        return false;
      default:
        LOG(FATAL) << "SecurityManager uses only one round trip";
    }
    return false;
  }

  const std::string& GetKey() const override {
    return spake_.GetUnverifiedKey();
  }

 private:
  crypto::P224EncryptedKeyExchange spake_;
};

class UnsecureKeyExchanger : public SecurityManager::KeyExchanger {
 public:
  explicit UnsecureKeyExchanger(const std::string& password)
      : password_(password) {}
  ~UnsecureKeyExchanger() override = default;

  // SecurityManager::KeyExchanger methods.
  const std::string& GetMessage() override { return password_; }

  bool ProcessMessage(const std::string& message, ErrorPtr* error) override {
    return true;
  }

  const std::string& GetKey() const override { return password_; }

 private:
  std::string password_;
};

}  // namespace

SecurityManager::SecurityManager(AuthManager* auth_manager,
                                 const std::set<PairingType>& pairing_modes,
                                 const std::string& embedded_code,
                                 bool disable_security,
                                 provider::TaskRunner* task_runner)
    : auth_manager_{auth_manager},
      is_security_disabled_(disable_security),
      pairing_modes_(pairing_modes),
      embedded_code_(embedded_code),
      task_runner_{task_runner} {
  CHECK(auth_manager_);
  CHECK_EQ(embedded_code_.empty(),
           std::find(pairing_modes_.begin(), pairing_modes_.end(),
                     PairingType::kEmbeddedCode) == pairing_modes_.end());
}

SecurityManager::~SecurityManager() {
  while (!pending_sessions_.empty())
    ClosePendingSession(pending_sessions_.begin()->first);
}

bool SecurityManager::CreateAccessToken(AuthType auth_type,
                                        const std::string& auth_code,
                                        AuthScope desired_scope,
                                        std::string* access_token,
                                        AuthScope* access_token_scope,
                                        base::TimeDelta* access_token_ttl,
                                        ErrorPtr* error) {
  switch (auth_type) {
    case AuthType::kAnonymous:
      break;
    case AuthType::kPairing:
      if (!IsValidPairingCode(auth_code)) {
        Error::AddTo(error, FROM_HERE, errors::kDomain,
                     errors::kInvalidAuthCode, "Invalid authCode");
        return false;
      }
      break;
    default:
      Error::AddToPrintf(error, FROM_HERE, errors::kDomain,
                         errors::kInvalidAuthMode, "Unsupported auth mode");
      return false;
  }

  UserInfo user_info{desired_scope,
                     std::to_string(static_cast<int>(auth_type)) + "/" +
                         std::to_string(++last_user_id_)};
  base::TimeDelta ttl =
      base::TimeDelta::FromSeconds(kAccessTokenExpirationSeconds);

  if (access_token) {
    *access_token =
        Base64Encode(auth_manager_->CreateAccessToken(user_info, ttl));
  }

  if (access_token_scope)
    *access_token_scope = user_info.scope();

  if (access_token_ttl)
    *access_token_ttl = ttl;

  return true;
}

bool SecurityManager::ParseAccessToken(const std::string& token,
                                       UserInfo* user_info,
                                       ErrorPtr* error) const {
  std::vector<uint8_t> decoded;
  if (!Base64Decode(token, &decoded)) {
    Error::AddToPrintf(error, FROM_HERE, errors::kDomain,
                       errors::kInvalidAuthorization,
                       "Invalid token encoding: %s", token.c_str());
    return false;
  }

  return auth_manager_->ParseAccessToken(decoded, user_info, error);
}

std::set<PairingType> SecurityManager::GetPairingTypes() const {
  return pairing_modes_;
}

std::set<CryptoType> SecurityManager::GetCryptoTypes() const {
  std::set<CryptoType> result{CryptoType::kSpake_p224};
  if (is_security_disabled_)
    result.insert(CryptoType::kNone);
  return result;
}

std::string SecurityManager::ClaimRootClientAuthToken(ErrorPtr* error) {
  return Base64Encode(auth_manager_->ClaimRootClientAuthToken(
      RootClientTokenOwner::kClient, error));
}

bool SecurityManager::ConfirmClientAuthToken(const std::string& token,
                                             ErrorPtr* error) {
  std::vector<uint8_t> token_decoded;
  if (!Base64Decode(token, &token_decoded)) {
    Error::AddToPrintf(error, FROM_HERE, errors::kDomain,
                       errors::kInvalidFormat,
                       "Invalid auth token string: '%s'", token.c_str());
    return false;
  }
  return auth_manager_->ConfirmClientAuthToken(token_decoded, error);
}

bool SecurityManager::IsValidPairingCode(const std::string& auth_code) const {
  if (is_security_disabled_)
    return true;
  std::vector<uint8_t> auth_decoded;
  if (!Base64Decode(auth_code, &auth_decoded))
    return false;
  for (const auto& session : confirmed_sessions_) {
    const std::string& key = session.second->GetKey();
    const std::string& id = session.first;
    if (auth_decoded ==
        HmacSha256(std::vector<uint8_t>(key.begin(), key.end()),
                   std::vector<uint8_t>(id.begin(), id.end()))) {
      pairing_attemts_ = 0;
      block_pairing_until_ = base::Time{};
      return true;
    }
  }
  LOG(ERROR) << "Attempt to authenticate with invalide code.";
  return false;
}

bool SecurityManager::StartPairing(PairingType mode,
                                   CryptoType crypto,
                                   std::string* session_id,
                                   std::string* device_commitment,
                                   ErrorPtr* error) {
  if (!CheckIfPairingAllowed(error))
    return false;

  if (std::find(pairing_modes_.begin(), pairing_modes_.end(), mode) ==
      pairing_modes_.end()) {
    Error::AddTo(error, FROM_HERE, errors::kDomain, errors::kInvalidParams,
                 "Pairing mode is not enabled");
    return false;
  }

  std::string code;
  switch (mode) {
    case PairingType::kEmbeddedCode:
      CHECK(!embedded_code_.empty());
      code = embedded_code_;
      break;
    case PairingType::kPinCode:
      code = base::StringPrintf("%04i", base::RandInt(0, 9999));
      break;
    default:
      Error::AddTo(error, FROM_HERE, errors::kDomain, errors::kInvalidParams,
                   "Unsupported pairing mode");
      return false;
  }

  std::unique_ptr<KeyExchanger> spake;
  switch (crypto) {
    case CryptoType::kSpake_p224:
      spake.reset(new Spakep224Exchanger(code));
      break;
    case CryptoType::kNone:
      if (is_security_disabled_) {
        spake.reset(new UnsecureKeyExchanger(code));
        break;
      }
    // Fall through...
    default:
      Error::AddTo(error, FROM_HERE, errors::kDomain, errors::kInvalidParams,
                   "Unsupported crypto");
      return false;
  }

  // Allow only a single session at a time for now.
  while (!pending_sessions_.empty())
    ClosePendingSession(pending_sessions_.begin()->first);

  std::string session;
  do {
    session = base::GenerateGUID();
  } while (confirmed_sessions_.find(session) != confirmed_sessions_.end() ||
           pending_sessions_.find(session) != pending_sessions_.end());
  std::string commitment = spake->GetMessage();
  pending_sessions_.insert(std::make_pair(session, std::move(spake)));

  task_runner_->PostDelayedTask(
      FROM_HERE,
      base::Bind(base::IgnoreResult(&SecurityManager::ClosePendingSession),
                 weak_ptr_factory_.GetWeakPtr(), session),
      base::TimeDelta::FromMinutes(kPairingExpirationTimeMinutes));

  *session_id = session;
  *device_commitment = Base64Encode(commitment);
  LOG(INFO) << "Pairing code for session " << *session_id << " is " << code;
  // TODO(vitalybuka): Handle case when device can't start multiple pairing
  // simultaneously and implement throttling to avoid brute force attack.
  if (!on_start_.is_null()) {
    on_start_.Run(session, mode,
                  std::vector<uint8_t>{code.begin(), code.end()});
  }

  return true;
}

bool SecurityManager::ConfirmPairing(const std::string& session_id,
                                     const std::string& client_commitment,
                                     std::string* fingerprint,
                                     std::string* signature,
                                     ErrorPtr* error) {
  auto session = pending_sessions_.find(session_id);
  if (session == pending_sessions_.end()) {
    Error::AddToPrintf(error, FROM_HERE, errors::kDomain,
                       errors::kUnknownSession, "Unknown session id: '%s'",
                       session_id.c_str());
    return false;
  }

  std::vector<uint8_t> commitment;
  if (!Base64Decode(client_commitment, &commitment)) {
    ClosePendingSession(session_id);
    Error::AddToPrintf(
        error, FROM_HERE, errors::kDomain, errors::kInvalidFormat,
        "Invalid commitment string: '%s'", client_commitment.c_str());
    return false;
  }

  if (!session->second->ProcessMessage(
          std::string(commitment.begin(), commitment.end()), error)) {
    ClosePendingSession(session_id);
    Error::AddTo(error, FROM_HERE, errors::kDomain, errors::kCommitmentMismatch,
                 "Pairing code or crypto implementation mismatch");
    return false;
  }

  const std::string& key = session->second->GetKey();
  VLOG(3) << "KEY " << base::HexEncode(key.data(), key.size());

  const auto& certificate_fingerprint =
      auth_manager_->GetCertificateFingerprint();
  *fingerprint = Base64Encode(certificate_fingerprint);
  std::vector<uint8_t> cert_hmac = HmacSha256(
      std::vector<uint8_t>(key.begin(), key.end()), certificate_fingerprint);
  *signature = Base64Encode(cert_hmac);
  confirmed_sessions_.insert(
      std::make_pair(session->first, std::move(session->second)));
  task_runner_->PostDelayedTask(
      FROM_HERE,
      base::Bind(base::IgnoreResult(&SecurityManager::CloseConfirmedSession),
                 weak_ptr_factory_.GetWeakPtr(), session_id),
      base::TimeDelta::FromMinutes(kSessionExpirationTimeMinutes));
  ClosePendingSession(session_id);
  return true;
}

bool SecurityManager::CancelPairing(const std::string& session_id,
                                    ErrorPtr* error) {
  bool confirmed = CloseConfirmedSession(session_id);
  bool pending = ClosePendingSession(session_id);
  if (pending) {
    CHECK_GE(pairing_attemts_, 1);
    --pairing_attemts_;
  }
  CHECK(!confirmed || !pending);
  if (confirmed || pending)
    return true;
  Error::AddToPrintf(error, FROM_HERE, errors::kDomain, errors::kUnknownSession,
                     "Unknown session id: '%s'", session_id.c_str());
  return false;
}

std::string SecurityManager::CreateSessionId() {
  return Base64Encode(auth_manager_->CreateSessionId());
}

void SecurityManager::RegisterPairingListeners(
    const PairingStartListener& on_start,
    const PairingEndListener& on_end) {
  CHECK(on_start_.is_null() && on_end_.is_null());
  on_start_ = on_start;
  on_end_ = on_end;
}

bool SecurityManager::CheckIfPairingAllowed(ErrorPtr* error) {
  if (is_security_disabled_)
    return true;

  if (block_pairing_until_ > auth_manager_->Now()) {
    Error::AddTo(error, FROM_HERE, errors::kDomain, errors::kDeviceBusy,
                 "Too many pairing attempts");
    return false;
  }

  if (++pairing_attemts_ >= kMaxAllowedPairingAttemts) {
    LOG(INFO) << "Pairing blocked for" << kPairingBlockingTimeMinutes
              << "minutes.";
    block_pairing_until_ = auth_manager_->Now();
    block_pairing_until_ +=
        base::TimeDelta::FromMinutes(kPairingBlockingTimeMinutes);
  }

  return true;
}

bool SecurityManager::ClosePendingSession(const std::string& session_id) {
  // The most common source of these session_id values is the map containing
  // the sessions, which we're about to clear out.  Make a local copy.
  const std::string safe_session_id{session_id};
  const size_t num_erased = pending_sessions_.erase(safe_session_id);
  if (num_erased > 0 && !on_end_.is_null())
    on_end_.Run(safe_session_id);
  return num_erased != 0;
}

bool SecurityManager::CloseConfirmedSession(const std::string& session_id) {
  return confirmed_sessions_.erase(session_id) != 0;
}

}  // namespace privet
}  // namespace weave
