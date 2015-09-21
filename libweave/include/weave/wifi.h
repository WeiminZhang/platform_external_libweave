// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_WIFI_H_
#define LIBWEAVE_INCLUDE_WEAVE_WIFI_H_

#include <string>

#include <base/callback.h>
#include <weave/error.h>

namespace weave {

class Wifi {
 public:
  // Implementation should attempt to connect to the given network with the
  // given passphrase. Implementation should run either of callback.
  // Callback should be posted.
  virtual void ConnectToService(
      const std::string& ssid,
      const std::string& passphrase,
      const base::Closure& success_callback,
      const base::Callback<void(const Error*)>& error_callback) = 0;

  // Starts WiFi access point for wifi setup.
  virtual void EnableAccessPoint(const std::string& ssid) = 0;

  // Stops WiFi access point.
  virtual void DisableAccessPoint() = 0;

 protected:
  virtual ~Wifi() = default;
};

}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_WIFI_H_
