// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/privet/auth_manager.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <weave/settings.h>

#include "src/config.h"
#include "src/data_encoding.h"
#include "src/test/mock_clock.h"

using testing::Return;

namespace weave {
namespace privet {

class AuthManagerTest : public testing::Test {
 public:
  void SetUp() override {
    EXPECT_GE(auth_.GetAuthSecret().size(), 32u);
    EXPECT_GE(auth_.GetAccessSecret().size(), 32u);
    EXPECT_GE(auth_.GetCertificateFingerprint().size(), 32u);

    EXPECT_CALL(clock_, Now())
        .WillRepeatedly(Return(base::Time::FromTimeT(1410000000)));
  }

 protected:
  const std::vector<uint8_t> kSecret1{
      78, 40, 39, 68, 29, 19, 70, 86, 38, 61, 13, 55, 33, 32, 51, 52,
      34, 43, 97, 48, 8,  56, 11, 99, 50, 59, 24, 26, 31, 71, 76, 28};
  const std::vector<uint8_t> kSecret2{
      69, 53, 17, 37, 80, 73, 2,  5, 79, 64, 41, 57, 12, 54, 65, 63,
      72, 74, 93, 81, 20, 95, 89, 3, 94, 92, 27, 21, 49, 90, 36, 6};
  const std::vector<uint8_t> kFingerprint{
      22, 47, 23, 77, 42, 98, 96, 25,  83, 16, 9, 14, 91, 44, 15, 75,
      60, 62, 10, 18, 82, 35, 88, 100, 30, 45, 7, 46, 67, 84, 58, 85};

  test::MockClock clock_;
  AuthManager auth_{kSecret1, kFingerprint, kSecret2, &clock_};
};

TEST_F(AuthManagerTest, RandomSecret) {
  AuthManager auth{{}, {}, {}, &clock_};
  EXPECT_EQ(auth.GetAuthSecret().size(), 32u);
  EXPECT_EQ(auth.GetAccessSecret().size(), 32u);
}

TEST_F(AuthManagerTest, DifferentSecret) {
  AuthManager auth{kSecret2, {}, kSecret1};
  EXPECT_EQ(auth.GetAuthSecret().size(), 32u);
  EXPECT_EQ(auth.GetAccessSecret().size(), 32u);
  EXPECT_NE(auth_.GetAccessSecret(), auth.GetAccessSecret());
  EXPECT_NE(auth_.GetAuthSecret(), auth.GetAuthSecret());
}

TEST_F(AuthManagerTest, Constructor) {
  EXPECT_EQ(kSecret1, auth_.GetAuthSecret());
  EXPECT_EQ(kSecret2, auth_.GetAccessSecret());
  EXPECT_EQ(kFingerprint, auth_.GetCertificateFingerprint());
}

TEST_F(AuthManagerTest, CreateAccessToken) {
  EXPECT_EQ("UABRUHgcSZDry0bvIsoJv+WDQgEURQJjMjM0RgUaVArkgA==",
            Base64Encode(auth_.CreateAccessToken(
                UserInfo{AuthScope::kViewer, "234"}, {})));
  EXPECT_EQ("UL7YEruLg5QQRDIp2+u1cqCDQgEIRQJjMjU3RgUaVArkgA==",
            Base64Encode(auth_.CreateAccessToken(
                UserInfo{AuthScope::kManager, "257"}, {})));
  EXPECT_EQ("UPFGeZRanR1wLGYLP5ZDkXiDQgECRQJjNDU2RgUaVArkgA==",
            Base64Encode(auth_.CreateAccessToken(
                UserInfo{AuthScope::kOwner, "456"}, {})));
  auto new_time = clock_.Now() + base::TimeDelta::FromDays(11);
  EXPECT_CALL(clock_, Now()).WillRepeatedly(Return(new_time));
  EXPECT_EQ("UMm9KlF3OEtZFBmhScJpl4uDQgEORQJjMzQ1RgUaVBllAA==",
            Base64Encode(auth_.CreateAccessToken(
                UserInfo{AuthScope::kUser, "345"}, {})));
}

TEST_F(AuthManagerTest, CreateSameToken) {
  EXPECT_EQ(auth_.CreateAccessToken(UserInfo{AuthScope::kViewer, "555"}, {}),
            auth_.CreateAccessToken(UserInfo{AuthScope::kViewer, "555"}, {}));
}

TEST_F(AuthManagerTest, CreateTokenDifferentScope) {
  EXPECT_NE(auth_.CreateAccessToken(UserInfo{AuthScope::kViewer, "456"}, {}),
            auth_.CreateAccessToken(UserInfo{AuthScope::kOwner, "456"}, {}));
}

TEST_F(AuthManagerTest, CreateTokenDifferentUser) {
  EXPECT_NE(auth_.CreateAccessToken(UserInfo{AuthScope::kOwner, "456"}, {}),
            auth_.CreateAccessToken(UserInfo{AuthScope::kOwner, "789"}, {}));
}

TEST_F(AuthManagerTest, CreateTokenDifferentTime) {
  auto token = auth_.CreateAccessToken(UserInfo{AuthScope::kOwner, "567"}, {});
  EXPECT_CALL(clock_, Now())
      .WillRepeatedly(Return(base::Time::FromTimeT(1400000000)));
  EXPECT_NE(token,
            auth_.CreateAccessToken(UserInfo{AuthScope::kOwner, "567"}, {}));
}

TEST_F(AuthManagerTest, CreateTokenDifferentInstance) {
  EXPECT_NE(auth_.CreateAccessToken(UserInfo{AuthScope::kUser, "123"}, {}),
            AuthManager({}, {}).CreateAccessToken(
                UserInfo{AuthScope::kUser, "123"}, {}));
}

TEST_F(AuthManagerTest, ParseAccessToken) {
  // Multiple attempts with random secrets.
  const auto kStartTime = base::Time::FromTimeT(1412121212);
  for (size_t i = 0; i < 1000; ++i) {
    EXPECT_CALL(clock_, Now()).WillRepeatedly(Return(kStartTime));

    AuthManager auth{{}, {}, {}, &clock_};

    auto token = auth.CreateAccessToken(UserInfo{AuthScope::kUser, "5"},
                                        base::TimeDelta::FromSeconds(i));
    UserInfo user_info;
    EXPECT_FALSE(auth_.ParseAccessToken(token, &user_info, nullptr));
    EXPECT_TRUE(auth.ParseAccessToken(token, &user_info, nullptr));
    EXPECT_EQ(AuthScope::kUser, user_info.scope());
    EXPECT_EQ("5", user_info.user_id());

    EXPECT_CALL(clock_, Now())
        .WillRepeatedly(Return(kStartTime + base::TimeDelta::FromSeconds(i)));
    EXPECT_TRUE(auth.ParseAccessToken(token, &user_info, nullptr));

    EXPECT_CALL(clock_, Now())
        .WillRepeatedly(
            Return(kStartTime + base::TimeDelta::FromSeconds(i + 1)));
    EXPECT_FALSE(auth.ParseAccessToken(token, &user_info, nullptr));
  }
}

TEST_F(AuthManagerTest, GetRootClientAuthToken) {
  EXPECT_EQ("UK1ACOc3cWGjGBoTIX2bd3qCQgECRgMaVArkgA==",
            Base64Encode(auth_.GetRootClientAuthToken()));
}

TEST_F(AuthManagerTest, GetRootClientAuthTokenDifferentTime) {
  auto new_time = clock_.Now() + base::TimeDelta::FromDays(15);
  EXPECT_CALL(clock_, Now()).WillRepeatedly(Return(new_time));
  EXPECT_EQ("UBpNF8g/GbNUmAyHg1qqJr+CQgECRgMaVB6rAA==",
            Base64Encode(auth_.GetRootClientAuthToken()));
}

TEST_F(AuthManagerTest, GetRootClientAuthTokenDifferentSecret) {
  AuthManager auth{kSecret2, {}, kSecret1, &clock_};
  EXPECT_EQ("UFTBUcgd9d0HnPRnLeroN2mCQgECRgMaVArkgA==",
            Base64Encode(auth.GetRootClientAuthToken()));
}

TEST_F(AuthManagerTest, IsValidAuthToken) {
  EXPECT_TRUE(auth_.IsValidAuthToken(auth_.GetRootClientAuthToken(), nullptr));
  // Multiple attempts with random secrets.
  for (size_t i = 0; i < 1000; ++i) {
    AuthManager auth{{}, {}, {}, &clock_};

    auto token = auth.GetRootClientAuthToken();
    EXPECT_FALSE(auth_.IsValidAuthToken(token, nullptr));
    EXPECT_TRUE(auth.IsValidAuthToken(token, nullptr));
  }
}

class AuthManagerClaimTest : public testing::Test {
 public:
  void SetUp() override { EXPECT_EQ(auth_.GetAuthSecret().size(), 32u); }

  bool TestClaim(RootClientTokenOwner owner, RootClientTokenOwner claimer) {
    Config::Transaction change{&config_};
    change.set_root_client_token_owner(owner);
    change.Commit();
    return !auth_.ClaimRootClientAuthToken(claimer, nullptr).empty();
  }

 protected:
  Config config_{nullptr};
  AuthManager auth_{&config_, {}};
};

TEST_F(AuthManagerClaimTest, WithPreviosOwner) {
  EXPECT_DEATH(
      TestClaim(RootClientTokenOwner::kNone, RootClientTokenOwner::kNone), "");
  EXPECT_DEATH(
      TestClaim(RootClientTokenOwner::kClient, RootClientTokenOwner::kNone),
      "");
  EXPECT_DEATH(
      TestClaim(RootClientTokenOwner::kCloud, RootClientTokenOwner::kNone), "");
  EXPECT_TRUE(
      TestClaim(RootClientTokenOwner::kNone, RootClientTokenOwner::kClient));
  EXPECT_FALSE(
      TestClaim(RootClientTokenOwner::kClient, RootClientTokenOwner::kClient));
  EXPECT_FALSE(
      TestClaim(RootClientTokenOwner::kCloud, RootClientTokenOwner::kClient));
  EXPECT_TRUE(
      TestClaim(RootClientTokenOwner::kNone, RootClientTokenOwner::kCloud));
  EXPECT_TRUE(
      TestClaim(RootClientTokenOwner::kClient, RootClientTokenOwner::kCloud));
  EXPECT_TRUE(
      TestClaim(RootClientTokenOwner::kCloud, RootClientTokenOwner::kCloud));
}

TEST_F(AuthManagerClaimTest, NormalClaim) {
  auto token =
      auth_.ClaimRootClientAuthToken(RootClientTokenOwner::kCloud, nullptr);
  EXPECT_FALSE(auth_.IsValidAuthToken(token, nullptr));
  EXPECT_EQ(RootClientTokenOwner::kNone,
            config_.GetSettings().root_client_token_owner);

  EXPECT_TRUE(auth_.ConfirmClientAuthToken(token, nullptr));
  EXPECT_TRUE(auth_.IsValidAuthToken(token, nullptr));
  EXPECT_EQ(RootClientTokenOwner::kCloud,
            config_.GetSettings().root_client_token_owner);
}

TEST_F(AuthManagerClaimTest, DoubleConfirm) {
  auto token =
      auth_.ClaimRootClientAuthToken(RootClientTokenOwner::kCloud, nullptr);
  EXPECT_TRUE(auth_.ConfirmClientAuthToken(token, nullptr));
  EXPECT_TRUE(auth_.ConfirmClientAuthToken(token, nullptr));
}

TEST_F(AuthManagerClaimTest, DoubleClaim) {
  auto token1 =
      auth_.ClaimRootClientAuthToken(RootClientTokenOwner::kCloud, nullptr);
  auto token2 =
      auth_.ClaimRootClientAuthToken(RootClientTokenOwner::kCloud, nullptr);
  EXPECT_TRUE(auth_.ConfirmClientAuthToken(token1, nullptr));
  EXPECT_FALSE(auth_.ConfirmClientAuthToken(token2, nullptr));
}

TEST_F(AuthManagerClaimTest, TokenOverflow) {
  auto token =
      auth_.ClaimRootClientAuthToken(RootClientTokenOwner::kCloud, nullptr);
  for (size_t i = 0; i < 100; ++i)
    auth_.ClaimRootClientAuthToken(RootClientTokenOwner::kCloud, nullptr);
  EXPECT_FALSE(auth_.ConfirmClientAuthToken(token, nullptr));
}

TEST_F(AuthManagerClaimTest, CreateAccessTokenFromAuth) {
  std::vector<uint8_t> access_token;
  AuthScope scope;
  base::TimeDelta ttl;
  EXPECT_TRUE(auth_.CreateAccessTokenFromAuth(
      auth_.GetRootClientAuthToken(), base::TimeDelta::FromDays(1),
      &access_token, &scope, &ttl, nullptr));
  UserInfo user_info;
  EXPECT_TRUE(auth_.ParseAccessToken(access_token, &user_info, nullptr));
  EXPECT_EQ(scope, user_info.scope());
  EXPECT_FALSE(user_info.user_id().empty());
}

}  // namespace privet
}  // namespace weave
