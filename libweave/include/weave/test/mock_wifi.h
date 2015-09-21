// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_TEST_MOCK_WIFI_H_
#define LIBWEAVE_INCLUDE_WEAVE_TEST_MOCK_WIFI_H_

#include <weave/network.h>

#include <string>

#include <gmock/gmock.h>

namespace weave {
namespace test {

class MockWifi : public Wifi {
 public:
  MockWifi() {}
  ~MockWifi() override = default;

  MOCK_METHOD4(ConnectToService,
               void(const std::string&,
                    const std::string&,
                    const base::Closure&,
                    const base::Callback<void(const Error*)>&));
  MOCK_METHOD1(EnableAccessPoint, void(const std::string&));
  MOCK_METHOD0(DisableAccessPoint, void());
};

}  // namespace test
}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_TEST_MOCK_WIFI_H_
