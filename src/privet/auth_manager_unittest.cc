// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/privet/auth_manager.h"

#include <gtest/gtest.h>
#include <weave/settings.h>

#include "src/data_encoding.h"

namespace weave {
namespace privet {

class AuthManagerTest : public testing::Test {
 public:
  void SetUp() override {
    EXPECT_GE(auth_.GetSecret().size(), 32u);
    EXPECT_GE(auth_.GetCertificateFingerprint().size(), 32u);
  }

 protected:
  const base::Time time_ = base::Time::FromTimeT(1410000000);

  const std::vector<uint8_t> kSecret{69, 53, 17, 37, 80, 73, 2,  5,  79, 64, 41,
                                     57, 12, 54, 65, 63, 72, 74, 93, 81, 20, 95,
                                     89, 3,  94, 92, 27, 21, 49, 90, 36, 6};
  const std::vector<uint8_t> kSecret2{
      78, 40, 39, 68, 29, 19, 70, 86, 38, 61, 13, 55, 33, 32, 51, 52,
      34, 43, 97, 48, 8,  56, 11, 99, 50, 59, 24, 26, 31, 71, 76, 28};
  const std::vector<uint8_t> kFingerprint{
      22, 47, 23, 77, 42, 98, 96, 25,  83, 16, 9, 14, 91, 44, 15, 75,
      60, 62, 10, 18, 82, 35, 88, 100, 30, 45, 7, 46, 67, 84, 58, 85};

  AuthManager auth_{kSecret, kFingerprint};
};

TEST_F(AuthManagerTest, RandomSecret) {
  EXPECT_GE(auth_.GetSecret().size(), 32u);
}

TEST_F(AuthManagerTest, DifferentSecret) {
  AuthManager auth{kSecret2, {}};
  EXPECT_GE(auth.GetSecret().size(), 32u);
  EXPECT_NE(auth_.GetSecret(), auth.GetSecret());
}

TEST_F(AuthManagerTest, Constructor) {
  EXPECT_EQ(kSecret, auth_.GetSecret());
  EXPECT_EQ(kFingerprint, auth_.GetCertificateFingerprint());
}

TEST_F(AuthManagerTest, CreateAccessToken) {
  EXPECT_EQ("OUH2L2npY+Gzwjf9AnqigGSK3hxIVR+xX8/Cnu4DGf8wOjA6MTQxMDAwMDAwMA==",
            Base64Encode(auth_.CreateAccessToken(
                UserInfo{AuthScope::kNone, 123}, time_)));
  EXPECT_EQ("iZx0qgEHFF5lq+Q503GtgU0d6gLQ9TlLsU+DcFbZb2QxOjIzNDoxNDEwMDAwMDAw",
            Base64Encode(auth_.CreateAccessToken(
                UserInfo{AuthScope::kViewer, 234}, time_)));
  EXPECT_EQ("qAmlJykiPTnFljfOKSf3BUII9YZG8/ttzD76q+fII1YyOjM0NToxNDEwOTUwNDAw",
            Base64Encode(auth_.CreateAccessToken(
                UserInfo{AuthScope::kUser, 345},
                time_ + base::TimeDelta::FromDays(11))));
  EXPECT_EQ("fTjecsbwtYj6i8/qPJz900B8EMAjRqU8jLT9kfMoz0czOjQ1NjoxNDEwMDAwMDAw",
            Base64Encode(auth_.CreateAccessToken(
                UserInfo{AuthScope::kOwner, 456}, time_)));
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

TEST_F(AuthManagerTest, GetRootDeviceToken) {
  EXPECT_EQ("UFTBUcgd9d0HnPRnLeroN2mCQgECRgMaVArkgA==",
            Base64Encode(auth_.GetRootDeviceToken(time_)));
}

TEST_F(AuthManagerTest, GetRootDeviceTokenDifferentTime) {
  EXPECT_EQ("UGKqwMYGQNOd8jeYFDOsM02CQgECRgMaVB6rAA==",
            Base64Encode(auth_.GetRootDeviceToken(
                time_ + base::TimeDelta::FromDays(15))));
}

TEST_F(AuthManagerTest, GetRootDeviceTokenDifferentSecret) {
  AuthManager auth{kSecret2, {}};
  EXPECT_EQ("UK1ACOc3cWGjGBoTIX2bd3qCQgECRgMaVArkgA==",
            Base64Encode(auth.GetRootDeviceToken(time_)));
}

}  // namespace privet
}  // namespace weave
