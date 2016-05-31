// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

namespace standard_traits {
const char kTraits[] = R"({
  "lock": {
    "commands": {
      "setConfig": {
        "minimalRole": "user",
        "parameters": {
          "lockedState": {
            "type": "string",
            "enum": [ "locked", "unlocked" ]
          }
        },
        "errors": [ "jammed", "lockingNotSupported" ]
      }
    },
    "state": {
      "lockedState": {
        "type": "string",
        "enum": [ "locked", "unlocked", "partiallyLocked" ],
        "isRequired": true
      },
      "isLockingSupported": {
        "type": "boolean",
        "isRequired": true
      }
    }
  },
  "onOff": {
    "commands": {
      "setConfig": {
        "minimalRole": "user",
        "parameters": {
          "state": {
            "type": "string",
            "enum": [ "on", "off" ]
          }
        }
      }
    },
    "state": {
      "state": {
        "type": "string",
        "enum": [ "on", "off" ],
        "isRequired": true
      }
    }
  },
  "brightness": {
    "commands": {
      "setConfig": {
        "minimalRole": "user",
        "parameters": {
          "brightness": {
            "type": "number",
            "minimum": 0.0,
            "maximum": 1.0
          }
        }
      }
    },
    "state": {
      "brightness": {
        "isRequired": true,
        "type": "number",
        "minimum": 0.0,
        "maximum": 1.0
      }
    }
  },
  "colorTemp": {
    "commands": {
      "setConfig": {
        "minimalRole": "user",
        "parameters": {
          "colorTemp": {
            "type": "integer"
          }
        }
      }
    },
    "state": {
      "colorTemp": {
        "isRequired": true,
        "type": "integer"
      },
      "minColorTemp": {
        "isRequired": true,
        "type": "integer"
      },
      "maxColorTemp": {
        "isRequired": true,
        "type": "integer"
      }
    }
  },
  "colorXy": {
    "commands": {
      "setConfig": {
        "minimalRole": "user",
        "parameters": {
          "colorSetting": {
            "type": "object",
            "required": [
              "colorX",
              "colorY"
            ],
            "properties": {
              "colorX": {
                "type": "number",
                "minimum": 0.0,
                "maximum": 1.0
              },
              "colorY": {
                "type": "number",
                "minimum": 0.0,
                "maximum": 1.0
              }
            },
            "additionalProperties": false
          }
        },
        "errors": ["colorOutOfRange"]
      }
    },
    "state": {
      "colorSetting": {
        "type": "object",
        "isRequired": true,
        "required": [ "colorX", "colorY" ],
        "properties": {
          "colorX": {
            "type": "number",
            "minimum": 0.0,
            "maximum": 1.0
          },
          "colorY": {
            "type": "number",
            "minimum": 0.0,
            "maximum": 1.0
          }
        }
      },
      "colorCapRed": {
        "type": "object",
        "isRequired": true,
        "required": [ "colorX", "colorY" ],
        "properties": {
          "colorX": {
            "type": "number",
            "minimum": 0.0,
            "maximum": 1.0
          },
          "colorY": {
            "type": "number",
            "minimum": 0.0,
            "maximum": 1.0
          }
        }
      },
      "colorCapGreen": {
        "type": "object",
        "isRequired": true,
        "required": [ "colorX", "colorY" ],
        "properties": {
          "colorX": {
            "type": "number",
            "minimum": 0.0,
            "maximum": 1.0
          },
          "colorY": {
            "type": "number",
            "minimum": 0.0,
            "maximum": 1.0
          }
        }
      },
      "colorCapBlue": {
        "type": "object",
        "isRequired": true,
        "required": [ "colorX", "colorY" ],
        "properties": {
          "colorX": {
            "type": "number",
            "minimum": 0.0,
            "maximum": 1.0
          },
          "colorY": {
            "type": "number",
            "minimum": 0.0,
            "maximum": 1.0
          }
        }
      }
    }
  },
  "volume": {
    "commands": {
      "setConfig": {
        "minimalRole": "user",
        "parameters": {
          "volume": {
            "type": "integer",
            "minimum": 0,
            "maximum": 100
          },
          "isMuted": {
            "type": "boolean"
          }
        }
      }
    },
    "state": {
      "volume": {
        "isRequired": true,
        "type": "integer",
        "minimum": 0,
        "maximum": 100
      },
      "isMuted": {
        "isRequired": true,
        "type": "boolean"
      }
    }
  }
})";

const char kDefaultState[] = R"({
  "lock":{"isLockingSupported": true},
  "onOff":{"state": "on"},
  "brightness":{"brightness": 0.0},
  "volume":{"isMuted": true},
  "colorTemp":{"colorTemp": 0},
    "colorXy": {
    "colorSetting": {"colorX": 0.0, "colorY": 0.0},
    "colorCapRed":  {"colorX": 0.674, "colorY": 0.322},
    "colorCapGreen":{"colorX": 0.408, "colorY": 0.517},
    "colorCapBlue": {"colorX": 0.168, "colorY": 0.041}
  }
})";

const char kComponent[] = "testdevice";
}  // namespace standard_traits
