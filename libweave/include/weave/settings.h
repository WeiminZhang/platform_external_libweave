// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_SETTINGS_H_
#define LIBWEAVE_INCLUDE_WEAVE_SETTINGS_H_

#include <set>
#include <string>

#include <base/time/time.h>
#include <weave/privet.h>

namespace weave {

struct Settings {
  // Model specific information. Must be set by ConfigStore::LoadDefaults.
  std::string firmware_version;
  std::string oem_name;
  std::string model_name;
  std::string model_id;

  // Basic device information. Must be set from ConfigStore::LoadDefaults.
  std::string name;
  std::string description;
  std::string location;

  // OAuth 2.0 related options. Must be set from ConfigStore::LoadDefaults.
  std::string api_key;
  std::string client_id;
  std::string client_secret;

  // Options mirrored into "base" state.
  // Maximum role for local anonymous user.
  std::string local_anonymous_access_role;
  // If true, allows local discovery using DNS-SD.
  bool local_discovery_enabled{true};
  // If true, allows local pairing using Privet API.
  bool local_pairing_enabled{true};

  // Set of pairing modes supported by device.
  std::set<PairingType> pairing_modes;

  // Embedded code. Will be used only if pairing_modes contains kEmbeddedCode.
  std::string embedded_code;

  // Optional cloud information. Can be used for testing or debugging.
  std::string oauth_url;
  std::string service_url;

  // Cloud ID of the registered device. Empty of device is not registered.
  std::string cloud_id;

  // Internal options used by libweave. External code should not use them.
  base::TimeDelta polling_period;
  base::TimeDelta backup_polling_period;
  bool wifi_auto_setup_enabled{true};
  bool ble_setup_enabled{false};
  std::string refresh_token;
  std::string robot_account;
  std::string last_configured_ssid;
  std::string secret;
};

}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_SETTINGS_H_
