// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_NETWORK_H_
#define LIBWEAVE_INCLUDE_WEAVE_NETWORK_H_

#include <string>

#include <base/callback.h>
#include <weave/error.h>
#include <weave/stream.h>

namespace weave {

enum class NetworkState {
  kOffline = 0,
  kFailure,
  kConnecting,
  kConnected,
};

class Network {
 public:
  // A callback that interested parties can register to be notified of
  // connectivity changes. Changes may include but not limited: interface
  // up or down, new IP is assigned, cable is disconnected.
  using OnConnectionChangedCallback = base::Callback<void()>;

  virtual void AddOnConnectionChangedCallback(
      const OnConnectionChangedCallback& listener) = 0;

  // Returns current Internet connectivity state
  virtual NetworkState GetConnectionState() const = 0;

  // Opens bidirectional sockets and returns attached stream.
  virtual void OpenSslSocket(
      const std::string& host,
      uint16_t port,
      const base::Callback<void(std::unique_ptr<Stream>)>& success_callback,
      const base::Callback<void(const Error*)>& error_callback) = 0;

 protected:
  virtual ~Network() = default;
};

}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_NETWORK_H_
