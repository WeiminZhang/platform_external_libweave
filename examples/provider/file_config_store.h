// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_EXAMPLES_PROVIDER_FILE_CONFIG_STORE_H_
#define LIBWEAVE_EXAMPLES_PROVIDER_FILE_CONFIG_STORE_H_

#include <map>
#include <string>
#include <vector>

#include <weave/provider/config_store.h>

namespace weave {
namespace examples {

class FileConfigStore : public provider::ConfigStore {
 public:
  FileConfigStore(bool disable_security, const std::string& model_id);

  bool LoadDefaults(Settings* settings) override;
  std::string LoadSettings() override;
  void SaveSettings(const std::string& settings) override;

 private:
  const bool disable_security_;
  const std::string model_id_;
  const std::string settings_path_;
};

}  // namespace examples
}  // namespace weave

#endif  // LIBWEAVE_EXAMPLES_PROVIDER_FILE_CONFIG_STORE_H_
