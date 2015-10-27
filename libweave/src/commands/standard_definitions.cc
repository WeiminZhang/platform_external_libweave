// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/commands/standard_definitions.h"

namespace weave {

const char kStandardCommandDefs[] = R"({
  "base": {
    "updateBaseConfiguration": {
      "minimalRole": "manager",
      "parameters": {
        "localDiscoveryEnabled": "boolean",
        "localAnonymousAccessMaxRole": [ "none", "viewer", "user" ],
        "localPairingEnabled": "boolean"
      },
      "results": {}
    },
    "reboot": {
      "minimalRole": "user",
      "parameters": {},
      "results": {}
    },
    "identify": {
      "minimalRole": "user",
      "parameters": {},
      "results": {}
    },
    "updateDeviceInfo": {
      "minimalRole": "manager",
      "parameters": {
        "description": "string",
        "name": "string",
        "location": "string"
      },
      "results": {}
    }
  }
})";

}  // namespace weave
