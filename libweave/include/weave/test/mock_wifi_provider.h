// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_TEST_MOCK_WIFI_PROVIDER_H_
#define LIBWEAVE_INCLUDE_WEAVE_TEST_MOCK_WIFI_PROVIDER_H_

#include <weave/network_provider.h>

#include <string>

#include <gmock/gmock.h>

namespace weave {
namespace test {

class MockWifiProvider : public WifiProvider {
 public:
  MOCK_METHOD4(Connect,
               void(const std::string&,
                    const std::string&,
                    const base::Closure&,
                    const ErrorCallback&));
  MOCK_METHOD1(StartAccessPoint, void(const std::string&));
  MOCK_METHOD0(StopAccessPoint, void());
};

}  // namespace test
}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_TEST_MOCK_WIFI_PROVIDER_H_
