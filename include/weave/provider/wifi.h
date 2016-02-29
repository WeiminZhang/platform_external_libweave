// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_PROVIDER_WIFI_H_
#define LIBWEAVE_INCLUDE_WEAVE_PROVIDER_WIFI_H_

#include <string>

#include <base/callback.h>
#include <weave/error.h>

namespace weave {
namespace provider {

// This interface should be implemented by the user of libweave and
// provided during device creation in Device::Create(...)
// libweave will use this interface to get WiFi capabilities and
// configure WiFi on the device.
//
// If device does not support WiFi (e.g. Ethernet device), user code
// should supply nullptr pointer for WiFi interface at the device creation
// time.
//
// Implementation of Connect(...) method should connect to specified
// WiFi access point (identified by ssid) using supplied passphrase.
// If WiFi access point allows open connection, passphrase will contain
// empty string. libweave should be notified when connection is established
// through callback invocation.
//
// Implementation of StartAccessPoint(...) should start open WiFi access point
// according to WiFi specification using specified ssid. New AP should be
// available on 2.4Ghz WiFi band, and be open (allow connection without
// passphrase).
// Implementation of StopAccessPoint() should stop previously started WiFi
// access point.
//
// Implementations of IsWifi24Supported() and IsWifi50Supported() should
// return if true if respectively 2.4Ghz and 5.0Ghz WiFi bands are supported.
//
// Implementation of GetConnectedSsid() should return SSID of the WiFi network
// device is currently connected to.

// Interface with methods to control WiFi capability of the device.
class Wifi {
 public:
  // Connects to the given network with the given pass-phrase. Implementation
  // should post either of callbacks.
  virtual void Connect(const std::string& ssid,
                       const std::string& passphrase,
                       const DoneCallback& callback) = 0;

  // Starts WiFi access point for wifi setup.
  virtual void StartAccessPoint(const std::string& ssid) = 0;

  // Stops WiFi access point.
  virtual void StopAccessPoint() = 0;

  virtual bool IsWifi24Supported() const = 0;
  virtual bool IsWifi50Supported() const = 0;

  // Get SSID of the network device is connected.
  virtual std::string GetConnectedSsid() const = 0;

 protected:
  virtual ~Wifi() {}
};

}  // namespace provider
}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_PROVIDER_WIFI_H_
