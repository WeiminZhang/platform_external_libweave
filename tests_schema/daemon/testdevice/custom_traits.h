// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

namespace custom_traits {
const char kCustomTraits[] = R"({
  "_ledflasher": {
    "commands": {
      "animate": {
        "minimalRole": "user",
        "parameters": {
          "duration": {
            "type": "number",
            "minimum": 0.1,
            "maximum": 100.0
          },
          "type": {
            "type": "string",
            "enum": [ "none", "marquee_left", "marquee_right", "blink" ]
          }
        }
      }
    },
    "state": {
      "status": {
        "type": "string",
        "enum": [ "idle", "animating" ]
      }
    }
  }
})";

const char kLedflasherState[] = R"({
  "_ledflasher":{"status": "idle"}
})";

const char ledflasher[] = "ledflasher";
const char led1[] = "led1";
const char led2[] = "led2";
const char led3[] = "led3";
const char led4[] = "led4";
const char led5[] = "led5";
const size_t kLedCount = 5;
const char kLedComponentPrefix[] = "led";
}  // namespace custom_traits

