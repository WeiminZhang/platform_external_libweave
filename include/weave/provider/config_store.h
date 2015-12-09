// Copyright 2015 The Weave Authors. All rights reserved.
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

// This interface should be implemented by the user of libweave and
// provided during device creation in Device::Create(...)
// libweave will use this interface to get default settings and load / save
// settings to a persistent storage.
//
// Implementation of the LoadDefaults(...) method may load settings from
// a file or just hardcode defaults for this device.
// For example:
//   bool FileConfigStore::LoadDefaults(Settings* settings) {
//     settings->name = "My device";
//     settings->pairing_modes.insert(kPinCode);
//     // set all other required settings, see include/weave/settings.h
//     return true;
//   }
//
// Implementation of LoadSettings() method should load previously
// stored settings from the persistent storage (file, flash, etc).
// For example:
//   std::string FileConfigStore::LoadSettings() {
//     std::ifstream str("/var/lib/weave/weave_settings.json");
//     return std::string(std::istreambuf_iterator<char>(str),
//                        std::istreambuf_iterator<char>());
//   }
// If data stored encrypted (highly recommended), LoadSettings()
// implementation should decrypt the data before returning it to libweave.
//
// Implementation of SaveSettings(...) method should store data in the
// persistent storage (file, flash, etc).
// For example:
//   void FileConfigStore::SaveSettings(const std::string& settings) {
//     std::ofstream str(kSettingsPath);
//     str << settings;
//   }
// It is highly recommended to protected data using encryption with
// hardware backed key.
//
// See libweave/examples/provider/file_config_store.cc for a complete
// example.

// Interface with methods to read/write libweave settings, device state and
// commands definitions.
class ConfigStore {
 public:
  // Returns default settings. This settings used for a new device or after
  // a factory reset.
  virtual bool LoadDefaults(Settings* settings) = 0;

  // Returns settings saved by SaveSettings during last run of libweave.
  // Implementation should return data as-is without parsing or modifications.
  virtual std::string LoadSettings() = 0;

  // Saves settings. Implementation should save data as-is without parsing or
  // modifications. Data stored in settings can be sensitive, so it's highly
  // recommended to protect data, e.g. using encryption.
  virtual void SaveSettings(const std::string& settings) = 0;

 protected:
  virtual ~ConfigStore() {}
};

}  // namespace provider
}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_PROVIDER_CONFIG_STORE_H_
