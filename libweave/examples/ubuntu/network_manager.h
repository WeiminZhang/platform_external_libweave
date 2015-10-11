// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_EXAMPLES_UBUNTU_NETWORK_MANAGER_H_
#define LIBWEAVE_EXAMPLES_UBUNTU_NETWORK_MANAGER_H_

#include <string>
#include <vector>

#include <base/memory/weak_ptr.h>
#include <base/time/time.h>
#include <weave/provider/network.h>
#include <weave/provider/wifi.h>

namespace weave {

namespace provider {
class TaskRunner;
}

namespace examples {

// Basic weave::Network implementation.
// Production version of SSL socket needs secure server certificate check.
class NetworkImpl : public provider::Network, public provider::Wifi {
 public:
  explicit NetworkImpl(provider::TaskRunner* task_runner,
                       bool force_bootstrapping);
  ~NetworkImpl();

  // Network implementation.
  void AddConnectionChangedCallback(
      const ConnectionChangedCallback& callback) override;
  State GetConnectionState() const override;
  void OpenSslSocket(const std::string& host,
                     uint16_t port,
                     const OpenSslSocketCallback& callback) override;

  // Wifi implementation.
  void Connect(const std::string& ssid,
               const std::string& passphrase,
               const DoneCallback& callback) override;
  void StartAccessPoint(const std::string& ssid) override;
  void StopAccessPoint() override;

  static bool HasWifiCapability();

 private:
  void TryToConnect(const std::string& ssid,
                    const std::string& passphrase,
                    int pid,
                    base::Time until,
                    const DoneCallback& callback);
  void UpdateNetworkState();

  bool force_bootstrapping_{false};
  bool hostapd_started_{false};
  provider::TaskRunner* task_runner_{nullptr};
  std::vector<ConnectionChangedCallback> callbacks_;
  provider::Network::State network_state_{provider::Network::State::kOffline};

  base::WeakPtrFactory<NetworkImpl> weak_ptr_factory_{this};
};

}  // namespace examples
}  // namespace weave

#endif  // LIBWEAVE_EXAMPLES_UBUNTU_NETWORK_MANAGER_H_
