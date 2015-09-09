// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_TEST_MOCK_NETWORK_H_
#define LIBWEAVE_INCLUDE_WEAVE_TEST_MOCK_NETWORK_H_

#include <weave/network.h>

#include <string>

#include <gmock/gmock.h>

namespace weave {
namespace test {

class MockNetwork : public Network {
 public:
  MockNetwork() {}
  ~MockNetwork() override = default;

  MOCK_METHOD1(AddOnConnectionChangedCallback,
               void(const OnConnectionChangedCallback&));
  MOCK_METHOD4(ConnectToService,
               bool(const std::string&,
                    const std::string&,
                    const base::Closure&,
                    ErrorPtr*));
  MOCK_CONST_METHOD0(GetConnectionState, NetworkState());
  MOCK_METHOD1(EnableAccessPoint, void(const std::string&));
  MOCK_METHOD0(DisableAccessPoint, void());

  MOCK_METHOD4(OpenSslSocket,
               void(const std::string&,
                    uint16_t,
                    const base::Callback<void(std::unique_ptr<Stream>)>&,
                    const base::Callback<void(const Error*)>&));
};

}  // namespace test
}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_TEST_MOCK_NETWORK_H_
