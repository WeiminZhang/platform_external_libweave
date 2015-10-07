// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_PROVIDER_CONFIG_STORE_H_
#define LIBWEAVE_INCLUDE_WEAVE_PROVIDER_CONFIG_STORE_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include <base/callback.h>
#include <base/time/time.h>
#include <weave/enum_to_string.h>
#include <weave/settings.h>

namespace weave {
namespace provider {

// Interface with methods to read/write libweave settings, device state and
// commands definitions.
class ConfigStore {
 public:
  // Returns default settings. This settings used for a new device or after
  // a factory reset.
  virtual bool LoadDefaults(Settings* settings) = 0;

  // Returns settings saved by SaveSettings during last run of libWeave.
  // Implementation should return data as-is without parsing or modifications.
  virtual std::string LoadSettings() = 0;

  // Saves settings. Implementation should save data as-is without parsing or
  // modifications. Data stored in settings can be sensitive, so it's highly
  // recommended to protect data, e.g. using encryption.
  virtual void SaveSettings(const std::string& settings) = 0;

 protected:
  virtual ~ConfigStore() = default;
};

}  // namespace provider
}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_PROVIDER_CONFIG_STORE_H_
