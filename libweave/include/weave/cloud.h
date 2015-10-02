// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_CLOUD_H_
#define LIBWEAVE_INCLUDE_WEAVE_CLOUD_H_

#include <string>

#include <base/callback.h>
#include <base/values.h>
#include <weave/error.h>
#include <weave/settings.h>

namespace weave {

// See the DBus interface XML file for complete descriptions of these states.
enum class RegistrationStatus {
  kUnconfigured,        // We have no credentials.
  kConnecting,          // We have credentials but not yet connected.
  kConnected,           // We're registered and connected to the cloud.
  kInvalidCredentials,  // Our registration has been revoked.
};

class Cloud {
 public:
  using OnRegistrationChangedCallback =
      base::Callback<void(RegistrationStatus satus)>;

  // Sets callback which is called when registration state is changed.
  virtual void AddOnRegistrationChangedCallback(
      const OnRegistrationChangedCallback& callback) = 0;

  // Registers the device.
  // Returns a device ID on success.
  virtual std::string RegisterDevice(const std::string& ticket_id,
                                     ErrorPtr* error) = 0;

  // Updates basic device information.
  virtual void UpdateDeviceInfo(const std::string& name,
                                const std::string& description,
                                const std::string& location) = 0;

  // Updates base device config.
  virtual void UpdateBaseConfig(AuthScope anonymous_access_role,
                                bool local_discovery_enabled,
                                bool local_pairing_enabled) = 0;

  // Updates GCD service configuration. Usually for testing.
  virtual bool UpdateServiceConfig(const std::string& client_id,
                                   const std::string& client_secret,
                                   const std::string& api_key,
                                   const std::string& oauth_url,
                                   const std::string& service_url,
                                   ErrorPtr* error) = 0;

 protected:
  virtual ~Cloud() = default;
};

}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_CLOUD_H_
