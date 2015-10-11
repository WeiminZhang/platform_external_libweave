// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_PROVIDER_HTTP_CLIENT_H_
#define LIBWEAVE_INCLUDE_WEAVE_PROVIDER_HTTP_CLIENT_H_

#include <string>
#include <utility>
#include <vector>

#include <base/callback.h>
#include <weave/error.h>

namespace weave {
namespace provider {

class HttpClient {
 public:
  enum class Method {
    kGet,
    kPatch,
    kPost,
    kPut,
  };

  class Response {
   public:
    virtual int GetStatusCode() const = 0;
    virtual std::string GetContentType() const = 0;
    virtual std::string GetData() const = 0;

    virtual ~Response() = default;
  };

  using Headers = std::vector<std::pair<std::string, std::string>>;
  using SuccessCallback = base::Callback<void(const Response&)>;

  virtual void SendRequest(Method method,
                           const std::string& url,
                           const Headers& headers,
                           const std::string& data,
                           const SuccessCallback& success_callback,
                           const ErrorCallback& error_callback) = 0;

 protected:
  virtual ~HttpClient() = default;
};

}  // namespace provider
}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_PROVIDER_HTTP_CLIENT_H_
