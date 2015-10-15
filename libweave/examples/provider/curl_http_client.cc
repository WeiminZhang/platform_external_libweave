// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/provider/curl_http_client.h"

#include <base/bind.h>
#include <curl/curl.h>
#include <weave/provider/task_runner.h>
#include <weave/enum_to_string.h>

namespace weave {
namespace examples {

namespace {

struct ResponseImpl : public provider::HttpClient::Response {
  int GetStatusCode() const override { return status; }
  std::string GetContentType() const override { return content_type; }
  std::string GetData() const override { return data; }

  long status{0};
  std::string content_type;
  std::string data;
};

size_t WriteFunction(void* contents, size_t size, size_t nmemb, void* userp) {
  static_cast<std::string*>(userp)
      ->append(static_cast<const char*>(contents), size * nmemb);
  return size * nmemb;
}

}  // namespace

CurlHttpClient::CurlHttpClient(provider::TaskRunner* task_runner)
    : task_runner_{task_runner} {}

void CurlHttpClient::SendRequest(Method method,
                                 const std::string& url,
                                 const Headers& headers,
                                 const std::string& data,
                                 const SendRequestCallback& callback) {
  std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl{curl_easy_init(),
                                                           &curl_easy_cleanup};
  CHECK(curl);

  switch (method) {
    case Method::kGet:
    CHECK_EQ(CURLE_OK, curl_easy_setopt(curl.get(), CURLOPT_HTTPGET, 1L));
    break;
    case Method::kPost:
    CHECK_EQ(CURLE_OK, curl_easy_setopt(curl.get(), CURLOPT_HTTPPOST, 1L));
    break;
    case Method::kPatch:
    case Method::kPut:
      CHECK_EQ(CURLE_OK, curl_easy_setopt(curl.get(), CURLOPT_CUSTOMREQUEST,
                                          weave::EnumToString(method).c_str()));
      break;
  }

  CHECK_EQ(CURLE_OK, curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str()));

  curl_slist* chunk = nullptr;
  for (const auto& h : headers)
    chunk = curl_slist_append(chunk, (h.first + ": " + h.second).c_str());

  CHECK_EQ(CURLE_OK, curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, chunk));

  if (!data.empty() || method == Method::kPost) {
    CHECK_EQ(CURLE_OK,
             curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, data.c_str()));
  }

  std::unique_ptr<ResponseImpl> response{new ResponseImpl};
  CHECK_EQ(CURLE_OK,
           curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, &WriteFunction));
  CHECK_EQ(CURLE_OK,
           curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response->data));
  CHECK_EQ(CURLE_OK, curl_easy_setopt(curl.get(), CURLOPT_HEADERFUNCTION,
                                      &WriteFunction));
  CHECK_EQ(CURLE_OK, curl_easy_setopt(curl.get(), CURLOPT_HEADERDATA,
                                      &response->content_type));

  CURLcode res = curl_easy_perform(curl.get());
  if (chunk)
    curl_slist_free_all(chunk);

  ErrorPtr error;
  if (res != CURLE_OK) {
    Error::AddTo(&error, FROM_HERE, "curl", "curl_easy_perform_error",
                 curl_easy_strerror(res));
    return task_runner_->PostDelayedTask(
        FROM_HERE, base::Bind(callback, nullptr, base::Passed(&error)), {});
  }

  const std::string kContentType = "\r\nContent-Type:";
  auto pos = response->content_type.find(kContentType);
  if (pos == std::string::npos) {
    Error::AddTo(&error, FROM_HERE, "curl", "no_content_header",
                 "Content-Type header is missing");
    return task_runner_->PostDelayedTask(
        FROM_HERE, base::Bind(callback, nullptr, base::Passed(&error)), {});
  }
  pos += kContentType.size();
  auto pos_end = response->content_type.find("\r\n", pos);
  if (pos_end == std::string::npos) {
    pos_end = response->content_type.size();
  }

  response->content_type = response->content_type.substr(pos, pos_end);

  CHECK_EQ(CURLE_OK, curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE,
                                       &response->status));

  task_runner_->PostDelayedTask(
      FROM_HERE, base::Bind(callback, base::Passed(&response), nullptr), {});
}

}  // namespace examples
}  // namespace weave
