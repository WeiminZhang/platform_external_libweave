// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/privet/auth_manager.h"

#include <gtest/gtest.h>
#include <weave/settings.h>

namespace weave {
namespace privet {

class AuthManagerTest : public testing::Test {
 public:
  void SetUp() override {}

 protected:
  const base::Time time_ = base::Time::FromTimeT(1410000000);
  AuthManager auth_{{}, {}};
};

TEST_F(AuthManagerTest, RandomSecret) {
  EXPECT_GE(auth_.GetSecret().size(), 32u);
}

TEST_F(AuthManagerTest, DifferentSecret) {
  AuthManager auth{{}, {}};
  EXPECT_NE(auth_.GetSecret(), auth.GetSecret());
}

TEST_F(AuthManagerTest, Constructor) {
  std::vector<uint8_t> secret;
  std::vector<uint8_t> fingerpint;
  for (uint8_t i = 0; i < 32; ++i) {
    secret.push_back(i);
    fingerpint.push_back(i + 100);
  }

  AuthManager auth{secret, fingerpint};
  EXPECT_EQ(secret, auth.GetSecret());
  EXPECT_EQ(fingerpint, auth.GetCertificateFingerprint());
}

TEST_F(AuthManagerTest, CreateSameToken) {
  EXPECT_EQ(auth_.CreateAccessToken(UserInfo{AuthScope::kViewer, 555}, time_),
            auth_.CreateAccessToken(UserInfo{AuthScope::kViewer, 555}, time_));
}

TEST_F(AuthManagerTest, CreateTokenDifferentScope) {
  EXPECT_NE(auth_.CreateAccessToken(UserInfo{AuthScope::kViewer, 456}, time_),
            auth_.CreateAccessToken(UserInfo{AuthScope::kOwner, 456}, time_));
}

TEST_F(AuthManagerTest, CreateTokenDifferentUser) {
  EXPECT_NE(auth_.CreateAccessToken(UserInfo{AuthScope::kOwner, 456}, time_),
            auth_.CreateAccessToken(UserInfo{AuthScope::kOwner, 789}, time_));
}

TEST_F(AuthManagerTest, CreateTokenDifferentTime) {
  EXPECT_NE(auth_.CreateAccessToken(UserInfo{AuthScope::kOwner, 567}, time_),
            auth_.CreateAccessToken(UserInfo{AuthScope::kOwner, 567},
                                    base::Time::FromTimeT(1400000000)));
}

TEST_F(AuthManagerTest, CreateTokenDifferentInstance) {
  EXPECT_NE(auth_.CreateAccessToken(UserInfo{AuthScope::kUser, 123}, time_),
            AuthManager({}, {})
                .CreateAccessToken(UserInfo{AuthScope::kUser, 123}, time_));
}

TEST_F(AuthManagerTest, ParseAccessToken) {
  // Multiple attempts with random secrets.
  for (size_t i = 0; i < 1000; ++i) {
    AuthManager auth{{}, {}};

    auto token = auth.CreateAccessToken(UserInfo{AuthScope::kUser, 5}, time_);
    base::Time time2;
    EXPECT_EQ(AuthScope::kUser, auth.ParseAccessToken(token, &time2).scope());
    EXPECT_EQ(5u, auth.ParseAccessToken(token, &time2).user_id());
    // Token timestamp resolution is one second.
    EXPECT_GE(1, std::abs((time_ - time2).InSeconds()));
  }
}

}  // namespace privet
}  // namespace weave
