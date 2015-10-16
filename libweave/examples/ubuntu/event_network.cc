// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/ubuntu/event_network.h"

#include <weave/enum_to_string.h>

#include <base/bind.h>
#include <event2/dns.h>
#include <event2/bufferevent.h>

#include "examples/ubuntu/event_task_runner.h"
#include "examples/ubuntu/ssl_stream.h"

namespace weave {
namespace examples {

namespace {
const char kNetworkProbeHostname[] = "talk.google.com";
const int kNetworkProbePort = 5223;
}  // namespace

void EventNetworkImpl::Deleter::operator()(evdns_base* dns_base) {
  evdns_base_free(dns_base, 0);
}

void EventNetworkImpl::Deleter::operator()(bufferevent* bev) {
  bufferevent_free(bev);
}

EventNetworkImpl::EventNetworkImpl(EventTaskRunner* task_runner)
    : task_runner_(task_runner) {
  UpdateNetworkState();
}

void EventNetworkImpl::AddConnectionChangedCallback(
    const ConnectionChangedCallback& callback) {
  callbacks_.push_back(callback);
}

void EventNetworkImpl::UpdateNetworkState() {
  std::unique_ptr<bufferevent, Deleter> bev{
      bufferevent_socket_new(task_runner_->GetEventBase(), -1,
                             BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS)};
  bufferevent_setcb(
      bev.get(), nullptr, nullptr,
      [](struct bufferevent* buf, short events, void* ctx) {
        EventNetworkImpl* network = static_cast<EventNetworkImpl*>(ctx);
        std::unique_ptr<bufferevent, Deleter> bev{buf};
        if (events & BEV_EVENT_CONNECTED) {
          network->UpdateNetworkStateCallback(State::kOnline);
          return;
        }
        if (events & (BEV_EVENT_ERROR | BEV_EVENT_EOF | BEV_EVENT_TIMEOUT)) {
          int err = bufferevent_socket_get_dns_error(bev.get());
          if (err) {
            LOG(ERROR) << "network connect dns error: "
                       << evutil_gai_strerror(err);
          }
          network->UpdateNetworkStateCallback(State::kOffline);
          return;
        }
      },
      this);
  int err = bufferevent_socket_connect_hostname(bev.get(), dns_base_.get(),
                                                AF_INET, kNetworkProbeHostname,
                                                kNetworkProbePort);
  if (err) {
    LOG(ERROR) << " network connect socket error: " << evutil_gai_strerror(err);
    UpdateNetworkStateCallback(State::kOffline);
    return;
  }
  // release the bufferevent, so that the eventcallback can free it.
  bev.release();
}

void EventNetworkImpl::UpdateNetworkStateCallback(
    provider::Network::State state) {
  network_state_ = state;
  LOG(INFO) << "network state updated: " << weave::EnumToString(state);
  for (const auto& cb : callbacks_)
    cb.Run();
  // TODO(proppy): use netlink interface event instead of polling
  task_runner_->PostDelayedTask(
      FROM_HERE, base::Bind(&EventNetworkImpl::UpdateNetworkState,
                            weak_ptr_factory_.GetWeakPtr()),
      base::TimeDelta::FromSeconds(10));
}

weave::provider::Network::State EventNetworkImpl::GetConnectionState() const {
  return network_state_;
}

void EventNetworkImpl::OpenSslSocket(const std::string& host,
                                     uint16_t port,
                                     const OpenSslSocketCallback& callback) {
  // Connect to SSL port instead of upgrading to TLS.
  std::unique_ptr<SSLStream> tls_stream{new SSLStream{task_runner_}};

  if (tls_stream->Init(host, port)) {
    task_runner_->PostDelayedTask(
        FROM_HERE, base::Bind(callback, base::Passed(&tls_stream), nullptr),
        {});
  } else {
    ErrorPtr error;
    Error::AddTo(&error, FROM_HERE, "tls", "tls_init_failed",
                 "Failed to initialize TLS stream.");
    task_runner_->PostDelayedTask(
        FROM_HERE, base::Bind(callback, nullptr, base::Passed(&error)), {});
  }
}

}  // namespace examples
}  // namespace weave
