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

class ConfigStore {
 public:
  virtual bool LoadDefaults(Settings* settings) = 0;
  virtual std::string LoadSettings() = 0;
  virtual void SaveSettings(const std::string& settings) = 0;
  virtual void OnSettingsChanged(const Settings& settings) = 0;

  virtual std::string LoadBaseCommandDefs() = 0;
  virtual std::map<std::string, std::string> LoadCommandDefs() = 0;

  virtual std::string LoadBaseStateDefs() = 0;
  virtual std::string LoadBaseStateDefaults() = 0;

  virtual std::map<std::string, std::string> LoadStateDefs() = 0;
  virtual std::vector<std::string> LoadStateDefaults() = 0;

 protected:
  virtual ~ConfigStore() = default;
};

}  // namespace provider
}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_PROVIDER_CONFIG_STORE_H_
