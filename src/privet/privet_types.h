// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_PRIVET_PRIVET_TYPES_H_
#define LIBWEAVE_SRC_PRIVET_PRIVET_TYPES_H_

#include <string>

#include <base/logging.h>
#include <weave/error.h>
#include <weave/settings.h>

namespace weave {
namespace privet {

enum class CryptoType {
  kNone,
  kSpake_p224,
};

enum class AuthType {
  kAnonymous,
  kPairing,
  kLocal,
};

enum class WifiType {
  kWifi24,
  kWifi50,
};

class UserInfo {
 public:
  explicit UserInfo(AuthScope scope = AuthScope::kNone,
                    const std::string& user_id = {})
      : scope_{scope}, user_id_{scope == AuthScope::kNone ? "" : user_id} {}
  AuthScope scope() const { return scope_; }
  const std::string& user_id() const { return user_id_; }

 private:
  AuthScope scope_;
  std::string user_id_;
};

class ConnectionState final {
 public:
  enum Status {
    kDisabled,
    kUnconfigured,
    kConnecting,
    kOnline,
    kOffline,
  };

  explicit ConnectionState(Status status) : status_(status) {}
  explicit ConnectionState(ErrorPtr error)
      : status_(kOffline), error_(std::move(error)) {}

  Status status() const {
    CHECK(!error_);
    return status_;
  }

  bool IsStatusEqual(Status status) const {
    if (error_)
      return false;
    return status_ == status;
  }

  const Error* error() const { return error_.get(); }

 private:
  Status status_;
  ErrorPtr error_;
};

class SetupState final {
 public:
  enum Status {
    kNone,
    kInProgress,
    kSuccess,
  };

  explicit SetupState(Status status) : status_(status) {}
  explicit SetupState(ErrorPtr error)
      : status_(kNone), error_(std::move(error)) {}

  Status status() const {
    CHECK(!error_);
    return status_;
  }

  bool IsStatusEqual(Status status) const {
    if (error_)
      return false;
    return status_ == status;
  }

  const Error* error() const { return error_.get(); }

 private:
  Status status_;
  ErrorPtr error_;
};

}  // namespace privet
}  // namespace weave

#endif  // LIBWEAVE_SRC_PRIVET_PRIVET_TYPES_H_
