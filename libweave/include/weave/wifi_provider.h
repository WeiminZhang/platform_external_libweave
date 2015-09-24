// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_WIFI_PROVIDER_H_
#define LIBWEAVE_INCLUDE_WEAVE_WIFI_PROVIDER_H_

#include <string>

#include <base/callback.h>
#include <weave/error.h>

namespace weave {

// Interface with methods to control WiFi capability of the device.
class WifiProvider {
 public:
  // Connects to the given network with the given pass-phrase. Implementation
  // should post either of callbacks.
  virtual void Connect(const std::string& ssid,
                       const std::string& passphrase,
                       const SuccessCallback& success_callback,
                       const ErrorCallback& error_callback) = 0;

  // Starts WiFi access point for wifi setup.
  virtual void StartAccessPoint(const std::string& ssid) = 0;

  // Stops WiFi access point.
  virtual void StopAccessPoint() = 0;

 protected:
  virtual ~WifiProvider() = default;
};

}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_WIFI_PROVIDER_H_
