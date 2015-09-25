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
  std::string client_id;
  std::string client_secret;
  std::string api_key;
  std::string oauth_url;
  std::string service_url;
  std::string name;
  std::string description;
  std::string location;
  std::string local_anonymous_access_role;
  bool local_discovery_enabled{true};
  bool local_pairing_enabled{true};
  std::string firmware_version;
  std::string oem_name;
  std::string model_name;
  std::string model_id;
  base::TimeDelta polling_period;
  base::TimeDelta backup_polling_period;

  bool wifi_auto_setup_enabled{true};
  bool ble_setup_enabled{false};
  std::set<PairingType> pairing_modes;
  std::string embedded_code;

  std::string device_id;
  std::string refresh_token;
  std::string robot_account;
  std::string last_configured_ssid;
};

}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_SETTINGS_H_
