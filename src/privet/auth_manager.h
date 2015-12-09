// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_PRIVET_AUTH_MANAGER_H_
#define LIBWEAVE_SRC_PRIVET_AUTH_MANAGER_H_

#include <string>
#include <vector>

#include <base/time/time.h>
#include <weave/error.h>

#include "src/privet/privet_types.h"

namespace weave {
namespace privet {

class AuthManager {
 public:
  AuthManager(const std::vector<uint8_t>& secret,
              const std::vector<uint8_t>& certificate_fingerprint);
  ~AuthManager();

  std::vector<uint8_t> CreateAccessToken(const UserInfo& user_info,
                                         const base::Time& time);
  UserInfo ParseAccessToken(const std::vector<uint8_t>& token,
                            base::Time* time) const;

  const std::vector<uint8_t>& GetSecret() const { return secret_; }
  const std::vector<uint8_t>& GetCertificateFingerprint() const {
    return certificate_fingerprint_;
  }
  std::vector<uint8_t> GetRootDeviceToken(const base::Time& time) const;

 private:
  std::vector<uint8_t> secret_;
  std::vector<uint8_t> certificate_fingerprint_;

  DISALLOW_COPY_AND_ASSIGN(AuthManager);
};

}  // namespace privet
}  // namespace weave

#endif  // LIBWEAVE_SRC_PRIVET_AUTH_MANAGER_H_
