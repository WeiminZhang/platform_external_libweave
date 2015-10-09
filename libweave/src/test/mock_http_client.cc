// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <weave/provider/test/mock_http_client.h>

#include <memory>
#include <string>

namespace weave {
namespace provider {
namespace test {

void MockHttpClient::SendRequest(const std::string& method,
                                 const std::string& url,
                                 const Headers& headers,
                                 const std::string& data,
                                 const SuccessCallback& success_callback,
                                 const ErrorCallback& error_callback) {
  ErrorPtr error;
  std::unique_ptr<Response> response{
      MockSendRequest(method, url, headers, data, &error)};
  if (response) {
    success_callback.Run(*response);
  } else {
    error_callback.Run(error.get());
  }
}

}  // namespace test
}  // namespace provider
}  // namespace weave
