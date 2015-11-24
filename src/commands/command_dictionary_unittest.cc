// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/commands/command_dictionary.h"

#include <gtest/gtest.h>

#include "src/commands/unittest_utils.h"

namespace weave {

using test::CreateDictionaryValue;
using test::IsEqualValue;

TEST(CommandDictionary, Empty) {
  CommandDictionary dict;
  EXPECT_TRUE(dict.IsEmpty());
  EXPECT_EQ(nullptr, dict.FindCommand("robot.jump"));
}

TEST(CommandDictionary, LoadCommands) {
  auto json = CreateDictionaryValue(R"({
    'robot': {
      'jump': {
        'parameters': {
          'height': 'integer',
          '_jumpType': ['_withAirFlip', '_withSpin', '_withKick']
        },
        'progress': {
          'progress': 'integer'
        },
        'results': {}
      }
    }
  })");
  CommandDictionary dict;
  EXPECT_TRUE(dict.LoadCommands(*json, nullptr));
  EXPECT_EQ(1, dict.GetSize());
  EXPECT_NE(nullptr, dict.FindCommand("robot.jump"));
  json = CreateDictionaryValue(R"({
    'base': {
      'reboot': {
        'parameters': {'delay': 'integer'}
      },
      'shutdown': {
      }
    }
  })");
  EXPECT_TRUE(dict.LoadCommands(*json, nullptr));
  EXPECT_EQ(3, dict.GetSize());
  EXPECT_NE(nullptr, dict.FindCommand("robot.jump"));
  EXPECT_NE(nullptr, dict.FindCommand("base.reboot"));
  EXPECT_NE(nullptr, dict.FindCommand("base.shutdown"));
  EXPECT_EQ(nullptr, dict.FindCommand("foo.bar"));
}

TEST(CommandDictionary, LoadCommands_Failures) {
  CommandDictionary dict;
  ErrorPtr error;

  // Command definition is not an object.
  auto json = CreateDictionaryValue("{'robot':{'jump':0}}");
  EXPECT_FALSE(dict.LoadCommands(*json, &error));
  EXPECT_EQ("type_mismatch", error->GetCode());
  error.reset();

  // Package definition is not an object.
  json = CreateDictionaryValue("{'robot':'blah'}");
  EXPECT_FALSE(dict.LoadCommands(*json, &error));
  EXPECT_EQ("type_mismatch", error->GetCode());
  error.reset();

  // Empty command name.
  json = CreateDictionaryValue("{'robot':{'':{'parameters':{},'results':{}}}}");
  EXPECT_FALSE(dict.LoadCommands(*json, &error));
  EXPECT_EQ("invalid_command_name", error->GetCode());
  error.reset();
}

TEST(CommandDictionaryDeathTest, LoadCommands_Redefine) {
  // Redefine commands.
  CommandDictionary dict;
  ErrorPtr error;
  auto json = CreateDictionaryValue("{'robot':{'jump':{}}}");
  dict.LoadCommands(*json, nullptr);
  ASSERT_DEATH(dict.LoadCommands(*json, &error),
               ".*Definition for command 'robot.jump' overrides an "
               "earlier definition");
}

TEST(CommandDictionary, GetCommandsAsJson) {
  auto json = CreateDictionaryValue(R"({
    'base': {
      'reboot': {
        'parameters': {'delay': {'minimum': 10}},
        'results': {}
      }
    },
    'robot': {
      '_jump': {
        'parameters': {'_height': 'integer'},
        'minimalRole': 'user'
      }
    }
  })");
  CommandDictionary dict;
  dict.LoadCommands(*json, nullptr);

  json = dict.GetCommandsAsJson(nullptr);
  ASSERT_NE(nullptr, json.get());
  auto expected = R"({
    'base': {
      'reboot': {
        'parameters': {'delay': {'minimum': 10}},
        'results': {}
      }
    },
    'robot': {
      '_jump': {
        'parameters': {'_height': 'integer'},
        'minimalRole': 'user'
      }
    }
  })";
  EXPECT_JSON_EQ(expected, *json);
}

TEST(CommandDictionary, LoadWithPermissions) {
  CommandDictionary base_dict;
  auto json = CreateDictionaryValue(R"({
    'base': {
      'command1': {
        'parameters': {},
        'results': {}
      },
      'command2': {
        'minimalRole': 'viewer',
        'parameters': {},
        'results': {}
      },
      'command3': {
        'minimalRole': 'user',
        'parameters': {},
        'results': {}
      },
      'command4': {
        'minimalRole': 'manager',
        'parameters': {},
        'results': {}
      },
      'command5': {
        'minimalRole': 'owner',
        'parameters': {},
        'results': {}
      }
    }
  })");
  EXPECT_TRUE(base_dict.LoadCommands(*json, nullptr));

  auto cmd = base_dict.FindCommand("base.command1");
  ASSERT_NE(nullptr, cmd);
  EXPECT_EQ(UserRole::kUser, cmd->GetMinimalRole());

  cmd = base_dict.FindCommand("base.command2");
  ASSERT_NE(nullptr, cmd);
  EXPECT_EQ(UserRole::kViewer, cmd->GetMinimalRole());

  cmd = base_dict.FindCommand("base.command3");
  ASSERT_NE(nullptr, cmd);
  EXPECT_EQ(UserRole::kUser, cmd->GetMinimalRole());

  cmd = base_dict.FindCommand("base.command4");
  ASSERT_NE(nullptr, cmd);
  EXPECT_EQ(UserRole::kManager, cmd->GetMinimalRole());

  cmd = base_dict.FindCommand("base.command5");
  ASSERT_NE(nullptr, cmd);
  EXPECT_EQ(UserRole::kOwner, cmd->GetMinimalRole());
}

}  // namespace weave
