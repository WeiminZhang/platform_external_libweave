// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/commands/command_manager.h"

#include <map>

#include <base/json/json_writer.h>
#include <gtest/gtest.h>
#include <weave/provider/test/mock_config_store.h>
#include <weave/test/unittest_utils.h>

#include "src/bind_lambda.h"

using testing::Return;

namespace weave {

using test::CreateDictionaryValue;

namespace {

const char kTestVendorCommands[] = R"({
  "robot": {
    "_jump": {
      "parameters": {"height": "integer"},
      "results": {}
    },
    "_speak": {
      "parameters": {"phrase": "string"},
      "results": {}
    }
  }
})";

const char kTestTestCommands[] = R"({
  "test": {
    "_yo": {
      "parameters": {"name": "string"},
      "results": {}
    }
  }
})";

}  // namespace

TEST(CommandManager, Empty) {
  CommandManager manager;
  EXPECT_TRUE(manager.GetCommandDictionary().IsEmpty());
}

TEST(CommandManager, LoadCommandsDict) {
  CommandManager manager;
  auto json = CreateDictionaryValue(kTestVendorCommands);
  EXPECT_TRUE(manager.LoadCommands(*json, nullptr));
}

TEST(CommandManager, LoadCommandsJson) {
  CommandManager manager;

  // Load device-supported commands.
  auto json_str = R"({
    "base": {
      "reboot": {
        "parameters": {"delay": "integer"},
        "results": {}
      }
    },
    "robot": {
      "_jump": {
        "parameters": {"height": "integer"},
        "results": {}
      }
    }
  })";
  EXPECT_TRUE(manager.LoadCommands(json_str, nullptr));
  EXPECT_EQ(2u, manager.GetCommandDictionary().GetSize());
  EXPECT_NE(nullptr, manager.GetCommandDictionary().FindCommand("base.reboot"));
  EXPECT_NE(nullptr, manager.GetCommandDictionary().FindCommand("robot._jump"));
}

TEST(CommandManager, ShouldLoadStandardAndTestDefinitions) {
  CommandManager manager;
  ASSERT_TRUE(manager.LoadCommands(kTestVendorCommands, nullptr));
  ASSERT_TRUE(manager.LoadCommands(kTestTestCommands, nullptr));
  EXPECT_EQ(3u, manager.GetCommandDictionary().GetSize());
  EXPECT_NE(nullptr, manager.GetCommandDictionary().FindCommand("robot._jump"));
  EXPECT_NE(nullptr,
            manager.GetCommandDictionary().FindCommand("robot._speak"));
  EXPECT_NE(nullptr, manager.GetCommandDictionary().FindCommand("test._yo"));
}

}  // namespace weave
