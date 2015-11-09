// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/provider/event_http_client.h"
#include "examples/provider/event_task_runner.h"


#include <base/bind.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <weave/enum_to_string.h>

#include "examples/provider/event_deleter.h"

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

class EventHttpResponse : public weave::provider::HttpClient::Response {
 public:
  int GetStatusCode() const override { return status; }
  std::string GetContentType() const override { return content_type; }
  std::string GetData() const { return data; }

  int status;
  std::string content_type;
  std::string data;
};

struct EventRequestState {
  TaskRunner* task_runner_;
  EventPtr<evhttp_uri> http_uri_;
  EventPtr<evhttp_connection> evcon_;
  HttpClient::SendRequestCallback callback_;
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
        FROM_HERE, base::Bind(state->callback_, nullptr, base::Passed(&error)),
        {});
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
      FROM_HERE, base::Bind(state->callback_, base::Passed(&response), nullptr),
      {});
}

}  // namespace

EventHttpClient::EventHttpClient(EventTaskRunner* task_runner)
    : task_runner_{task_runner} {}

void EventHttpClient::SendRequest(Method method,
                                  const std::string& url,
                                  const Headers& headers,
                                  const std::string& data,
                                  const SendRequestCallback& callback) {
  evhttp_cmd_type method_id;
  CHECK(weave::StringToEnum(weave::EnumToString(method), &method_id));
  EventPtr<evhttp_uri> http_uri{evhttp_uri_parse(url.c_str())};
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
  EventPtr<evhttp_connection> conn{evhttp_connection_base_bufferevent_new(
      task_runner_->GetEventBase(), NULL, bev, host, port)};
  CHECK(conn);
  EventPtr<evhttp_request> req{evhttp_request_new(
      &RequestDoneCallback,
      new EventRequestState{task_runner_, std::move(http_uri), std::move(conn),
                            callback})};
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
                     "request failed: %s %s", EnumToString(method).c_str(),
                     url.c_str());
  task_runner_->PostDelayedTask(
      FROM_HERE, base::Bind(callback, nullptr, base::Passed(&error)), {});
}

}  // namespace examples
}  // namespace weave
