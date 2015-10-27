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

const char kStandardStateDefs[] = R"({
  "base": {
    "firmwareVersion": "string",
    "localDiscoveryEnabled": "boolean",
    "localAnonymousAccessMaxRole": [ "none", "viewer", "user" ],
    "localPairingEnabled": "boolean",
    "network": {
      "properties": {
        "name": "string"
      }
    }
  }
})";

const char kStandardStateDefaults[] = R"({
  "base": {
    "firmwareVersion": "unknown",
    "localDiscoveryEnabled": false,
    "localAnonymousAccessMaxRole": "none",
    "localPairingEnabled": false
  }
})";

}  // namespace weave
