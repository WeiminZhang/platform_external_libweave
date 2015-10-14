// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_EXAMPLES_UBUNTU_NETLINK_MANAGER_H_
#define LIBWEAVE_EXAMPLES_UBUNTU_NETLINK_MANAGER_H_

#include <weave/provider/network.h>

#include <base/memory/weak_ptr.h>

struct nl_sock;
struct nl_cache;
struct rtnl_link;

namespace weave {

namespace provider {
class TaskRunner;
}

namespace examples {

class NetlinkNetworkImpl : public weave::provider::Network {
 public:
  explicit NetlinkNetworkImpl(weave::provider::TaskRunner* task_runner_);
  void AddConnectionChangedCallback(
      const ConnectionChangedCallback& callback) override;
  State GetConnectionState() const override;
  void OpenSslSocket(const std::string& host,
                     uint16_t port,
                     const OpenSslSocketCallback& callback) override;

 private:
  class Deleter {
   public:
    void operator()(nl_sock* s);
    void operator()(nl_cache* c);
    void operator()(rtnl_link* l);
  };
  void UpdateNetworkState();
  State GetInterfaceState(int if_index);
  provider::TaskRunner* task_runner_{nullptr};
  std::vector<ConnectionChangedCallback> callbacks_;
  provider::Network::State network_state_{provider::Network::State::kOffline};
  std::unique_ptr<nl_sock, Deleter> nl_sock_;
  std::unique_ptr<nl_cache, Deleter> nl_cache_;
  base::WeakPtrFactory<NetlinkNetworkImpl> weak_ptr_factory_{this};
};

}  // namespace examples
}  // namespace weave

#endif  // LIBWEAVE_EXAMPLES_UBUNTU_NETLINK_MANAGER_H_
