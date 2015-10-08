// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_PROVIDER_TEST_MOCK_HTTP_SERVER_H_
#define LIBWEAVE_INCLUDE_WEAVE_PROVIDER_TEST_MOCK_HTTP_SERVER_H_

#include <weave/provider/http_server.h>

#include <string>
#include <vector>

#include <base/callback.h>

namespace weave {
namespace provider {
namespace test {

class MockHttpServer : public HttpServer {
 public:
  MOCK_METHOD2(AddRequestHandler,
               void(const std::string&, const OnRequestCallback&));

  MOCK_CONST_METHOD0(GetHttpPort, uint16_t());
  MOCK_CONST_METHOD0(GetHttpsPort, uint16_t());
  MOCK_CONST_METHOD0(GetHttpsCertificateFingerprint, std::vector<uint8_t>&());
};

}  // namespace test
}  // namespace provider
}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_PROVIDER_TEST_MOCK_HTTP_SERVER_H_
