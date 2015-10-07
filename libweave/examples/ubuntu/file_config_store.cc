// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/ubuntu/file_config_store.h"

#include <sys/stat.h>
#include <sys/utsname.h>

#include <fstream>
#include <map>
#include <string>
#include <vector>

namespace weave {
namespace examples {

const char kSettingsDir[] = "/var/lib/weave/";
const char kSettingsPath[] = "/var/lib/weave/weave_settings.json";
const char kCategory[] = "example";

FileConfigStore::FileConfigStore(bool disable_security)
    : disable_security_{disable_security} {}

bool FileConfigStore::LoadDefaults(Settings* settings) {
  char host_name[HOST_NAME_MAX] = {};
  gethostname(host_name, HOST_NAME_MAX);

  settings->name = host_name;
  settings->description = "";

  utsname uname_data;
  uname(&uname_data);

  settings->firmware_version = uname_data.sysname;
  settings->oem_name = "Unknown";
  settings->model_name = "Unknown";
  settings->model_id = "AAAAA";
  settings->pairing_modes = {PairingType::kEmbeddedCode};
  settings->embedded_code = "0000";
  settings->client_id = "58855907228.apps.googleusercontent.com";
  settings->client_secret = "eHSAREAHrIqPsHBxCE9zPPBi";
  settings->api_key = "AIzaSyDSq46gG-AxUnC3zoqD9COIPrjolFsMfMA";

  settings->disable_security = disable_security_;
  return true;
}

std::string FileConfigStore::LoadSettings() {
  LOG(INFO) << "Loading settings from " << kSettingsPath;
  std::ifstream str(kSettingsPath);
  return std::string(std::istreambuf_iterator<char>(str),
                     std::istreambuf_iterator<char>());
}

void FileConfigStore::SaveSettings(const std::string& settings) {
  CHECK(mkdir(kSettingsDir, S_IRWXU) == 0 || errno == EEXIST);
  LOG(INFO) << "Saving settings to " << kSettingsPath;
  std::ofstream str(kSettingsPath);
  str << settings;
}

}  // namespace examples
}  // namespace weave
