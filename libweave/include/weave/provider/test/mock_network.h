// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_PROVIDER_TEST_MOCK_NETWORK_H_
#define LIBWEAVE_INCLUDE_WEAVE_PROVIDER_TEST_MOCK_NETWORK_H_

#include <weave/provider/network.h>

#include <string>

#include <gmock/gmock.h>

namespace weave {
namespace provider {
namespace test {

class MockNetwork : public Network {
 public:
  MOCK_METHOD1(AddConnectionChangedCallback,
               void(const ConnectionChangedCallback&));
  MOCK_CONST_METHOD0(GetConnectionState, NetworkState());
  MOCK_METHOD4(OpenSslSocket,
               void(const std::string&,
                    uint16_t,
                    const OpenSslSocketSuccessCallback&,
                    const ErrorCallback&));
};

}  // namespace test
}  // namespace provider
}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_PROVIDER_TEST_MOCK_NETWORK_H_
