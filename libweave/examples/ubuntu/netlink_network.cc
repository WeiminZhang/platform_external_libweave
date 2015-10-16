// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/ubuntu/netlink_network.h"

#include <weave/provider/task_runner.h>
#include <base/bind.h>
#include <netlink/route/link.h>

#include "examples/ubuntu/ssl_stream.h"

namespace weave {
namespace examples {

void NetlinkNetworkImpl::Deleter::operator()(nl_sock* s) {
  nl_close(s);
  nl_socket_free(s);
}

void NetlinkNetworkImpl::Deleter::operator()(nl_cache* c) {
  nl_cache_put(c);
}

void NetlinkNetworkImpl::Deleter::operator()(rtnl_link* l) {
  rtnl_link_put(l);
}

NetlinkNetworkImpl::NetlinkNetworkImpl(weave::provider::TaskRunner* task_runner)
    : task_runner_(task_runner) {
  nl_sock_.reset(nl_socket_alloc());
  CHECK(nl_sock_);
  CHECK_EQ(nl_connect(nl_sock_.get(), NETLINK_ROUTE), 0);
  nl_cache* nl_cache;
  CHECK_EQ(rtnl_link_alloc_cache(nl_sock_.get(), AF_UNSPEC, &nl_cache), 0);
  nl_cache_.reset(nl_cache);
  UpdateNetworkState();
}

void NetlinkNetworkImpl::AddConnectionChangedCallback(
    const ConnectionChangedCallback& callback) {
  callbacks_.push_back(callback);
}

void NetlinkNetworkImpl::UpdateNetworkState() {
  // TODO(proppy): pick up interface with the default route instead of index 0.
  // TODO(proppy): test actual internet connection.
  // TODO(proppy): subscribe to interface changes instead of polling.
  network_state_ = GetInterfaceState(0);
  task_runner_->PostDelayedTask(
      FROM_HERE, base::Bind(&NetlinkNetworkImpl::UpdateNetworkState,
                            weak_ptr_factory_.GetWeakPtr()),
      base::TimeDelta::FromSeconds(10));
  for (const auto& cb : callbacks_)
    cb.Run();
}

weave::provider::Network::State NetlinkNetworkImpl::GetInterfaceState(
    int if_index) {
  auto refill_result = nl_cache_refill(nl_sock_.get(), nl_cache_.get());
  if (refill_result < 0) {
    LOG(ERROR) << "failed to refresh netlink cache: " << refill_result;
    return Network::State::kError;
  }
  std::unique_ptr<rtnl_link, Deleter> nl_link{
      rtnl_link_get(nl_cache_.get(), if_index)};
  if (!nl_link) {
    LOG(ERROR) << "failed to get interface 0";
    return Network::State::kError;
  }

  int state = rtnl_link_get_operstate(nl_link.get());
  switch (state) {
    case IF_OPER_DOWN:
    case IF_OPER_LOWERLAYERDOWN:
      return Network::State::kOffline;
    case IF_OPER_DORMANT:
      return Network::State::kConnecting;
    case IF_OPER_UP:
      return Network::State::kOnline;
    case IF_OPER_TESTING:
    case IF_OPER_NOTPRESENT:
    case IF_OPER_UNKNOWN:
    default:
      LOG(ERROR) << "unknown interface state: " << state;
      return Network::State::kError;
  }
}

weave::provider::Network::State NetlinkNetworkImpl::GetConnectionState() const {
  return network_state_;
}

void NetlinkNetworkImpl::OpenSslSocket(const std::string& host,
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
