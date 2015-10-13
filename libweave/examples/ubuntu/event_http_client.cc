// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/ubuntu/event_http_client.h"
#include "examples/ubuntu/event_task_runner.h"

#include <weave/enum_to_string.h>

#include <string>

#include <base/bind.h>

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/http.h>

// EventHttpClient based on libevent2 http-client sample
// TODO(proppy): https
// TODO(proppy): hostname validation
namespace weave {

namespace {
const weave::EnumToStringMap<evhttp_cmd_type>::Map kMapMethod[] = {
    {EVHTTP_REQ_GET, "GET"},        {EVHTTP_REQ_POST, "POST"},
    {EVHTTP_REQ_HEAD, "HEAD"},      {EVHTTP_REQ_PUT, "PUT"},
    {EVHTTP_REQ_PATCH, "PATCH"},    {EVHTTP_REQ_DELETE, "DELETE"},
    {EVHTTP_REQ_OPTIONS, "OPTIONS"}};
}  // namespace

template <>
EnumToStringMap<evhttp_cmd_type>::EnumToStringMap()
    : EnumToStringMap(kMapMethod) {}

using namespace provider;

namespace examples {

namespace {

class EventDeleter {
 public:
  void operator()(evhttp_uri* http_uri) { evhttp_uri_free(http_uri); }
  void operator()(evhttp_connection* conn) { evhttp_connection_free(conn); }
  void operator()(evhttp_request* req) { evhttp_request_free(req); }
};

class EventHttpResponse : public weave::provider::HttpClient::Response {
 public:
  int GetStatusCode() const override { return status; }
  std::string GetContentType() const override { return content_type; }
  const std::string& GetData() const { return data; }

  int status;
  std::string content_type;
  std::string data;
};

struct EventRequestState {
  TaskRunner* task_runner_;
  std::unique_ptr<evhttp_uri, EventDeleter> http_uri_;
  std::unique_ptr<evhttp_connection, EventDeleter> evcon_;
  HttpClient::SuccessCallback success_callback_;
  HttpClient::ErrorCallback error_callback_;
};

void RequestDoneCallback(evhttp_request* req, void* ctx) {
  std::unique_ptr<EventRequestState> state{
      static_cast<EventRequestState*>(ctx)};
  if (!req) {
    ErrorPtr error;
    auto err = EVUTIL_SOCKET_ERROR();
    Error::AddToPrintf(&error, FROM_HERE, "http_client", "request_failed",
                       "request failed: %s",
                       evutil_socket_error_to_string(err));
    state->task_runner_->PostDelayedTask(
        FROM_HERE, base::Bind(state->error_callback_, error.get()), {});
    return;
  }
  std::unique_ptr<EventHttpResponse> response{new EventHttpResponse()};
  response->status = evhttp_request_get_response_code(req);
  auto buffer = evhttp_request_get_input_buffer(req);
  auto length = evbuffer_get_length(buffer);
  response->data.resize(length);
  auto n = evbuffer_remove(buffer, &response->data[0], length);
  CHECK_EQ(n, int(length));
  state->task_runner_->PostDelayedTask(
      FROM_HERE, base::Bind(state->success_callback_, *response), {});
}

}  // namespace

EventHttpClient::EventHttpClient(EventTaskRunner* task_runner)
    : task_runner_{task_runner} {}

std::unique_ptr<provider::HttpClient::Response>
EventHttpClient::SendRequestAndBlock(const std::string& method,
                                     const std::string& url,
                                     const Headers& headers,
                                     const std::string& data,
                                     ErrorPtr* error) {
  return nullptr;
}

void EventHttpClient::SendRequest(const std::string& method,
                                  const std::string& url,
                                  const Headers& headers,
                                  const std::string& data,
                                  const SuccessCallback& success_callback,
                                  const ErrorCallback& error_callback) {
  evhttp_cmd_type method_id;
  CHECK(weave::StringToEnum(method, &method_id));
  std::unique_ptr<evhttp_uri, EventDeleter> http_uri{
      evhttp_uri_parse(url.c_str())};
  CHECK(http_uri);
  auto host = evhttp_uri_get_host(http_uri.get());
  CHECK(host);
  auto port = evhttp_uri_get_port(http_uri.get());
  if (port == -1)
    port = 80;
  std::string path{evhttp_uri_get_path(http_uri.get())};
  if (path.length() == 0) {
    path = "/";
  }
  std::string uri{path};
  auto query = evhttp_uri_get_query(http_uri.get());
  if (query) {
    uri = path + "?" + query;
  }
  auto bev = bufferevent_socket_new(task_runner_->GetEventBase(), -1,
                                    BEV_OPT_CLOSE_ON_FREE);
  CHECK(bev);
  std::unique_ptr<evhttp_connection, EventDeleter> conn{
      evhttp_connection_base_bufferevent_new(task_runner_->GetEventBase(), NULL,
                                             bev, host, port)};
  CHECK(conn);
  std::unique_ptr<evhttp_request, EventDeleter> req{evhttp_request_new(
      &RequestDoneCallback,
      new EventRequestState{task_runner_, std::move(http_uri), std::move(conn),
                            success_callback, error_callback})};
  CHECK(req);
  auto output_headers = evhttp_request_get_output_headers(req.get());
  evhttp_add_header(output_headers, "Host", host);
  for (auto& kv : headers)
    evhttp_add_header(output_headers, kv.first.c_str(), kv.second.c_str());
  if (!data.empty()) {
    auto output_buffer = evhttp_request_get_output_buffer(req.get());
    evbuffer_add(output_buffer, data.c_str(), data.length());
    evhttp_add_header(output_headers, "Content-Length",
                      std::to_string(data.length()).c_str());
  }
  auto res =
      evhttp_make_request(conn.get(), req.release(), method_id, uri.c_str());
  if (res >= 0)
    return;
  ErrorPtr error;
  Error::AddToPrintf(&error, FROM_HERE, "http_client", "request_failed",
                     "request failed: %s %s", method.c_str(), url.c_str());
  task_runner_->PostDelayedTask(FROM_HERE,
                                base::Bind(error_callback, error.get()), {});
}

}  // namespace examples
}  // namespace weave
