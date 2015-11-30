// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/commands/command_dictionary.h"

#include <gtest/gtest.h>
#include <weave/test/unittest_utils.h>

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
        'minimalRole': 'manager',
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
  EXPECT_EQ(1u, dict.GetSize());
  EXPECT_NE(nullptr, dict.FindCommand("robot.jump"));
  json = CreateDictionaryValue(R"({
    'base': {
      'reboot': {
        'minimalRole': 'owner',
        'parameters': {'delay': 'integer'}
      },
      'shutdown': {
        'minimalRole': 'user'
      }
    }
  })");
  EXPECT_TRUE(dict.LoadCommands(*json, nullptr));
  EXPECT_EQ(3u, dict.GetSize());
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

  // No 'minimalRole'.
  json = CreateDictionaryValue(R"({
    'base': {
      'reboot': {
        'parameters': {'delay': 'integer'}
      }
    }
  })");
  EXPECT_FALSE(dict.LoadCommands(*json, &error));
  EXPECT_EQ("invalid_minimal_role", error->GetCode());
  error.reset();

  // Invalid 'minimalRole'.
  json = CreateDictionaryValue(R"({
    'base': {
      'reboot': {
        'minimalRole': 'foo',
        'parameters': {'delay': 'integer'}
      }
    }
  })");
  EXPECT_FALSE(dict.LoadCommands(*json, &error));
  EXPECT_EQ("invalid_minimal_role", error->GetCode());
  error.reset();
}

TEST(CommandDictionaryDeathTest, LoadCommands_Redefine) {
  // Redefine commands.
  CommandDictionary dict;
  ErrorPtr error;
  auto json =
      CreateDictionaryValue("{'robot':{'jump':{'minimalRole': 'viewer'}}}");
  dict.LoadCommands(*json, nullptr);
  ASSERT_DEATH(dict.LoadCommands(*json, &error),
               ".*Definition for command 'robot.jump' overrides an "
               "earlier definition");
}

TEST(CommandDictionary, GetMinimalRole) {
  CommandDictionary base_dict;
  auto json = CreateDictionaryValue(R"({
    'base': {
      'command1': {
        'minimalRole': 'viewer',
        'parameters': {},
        'results': {}
      },
      'command2': {
        'minimalRole': 'user',
        'parameters': {},
        'results': {}
      },
      'command3': {
        'minimalRole': 'manager',
        'parameters': {},
        'results': {}
      },
      'command4': {
        'minimalRole': 'owner',
        'parameters': {},
        'results': {}
      }
    }
  })");
  EXPECT_TRUE(base_dict.LoadCommands(*json, nullptr));
  UserRole role;
  EXPECT_TRUE(base_dict.GetMinimalRole("base.command1", &role, nullptr));
  EXPECT_EQ(UserRole::kViewer, role);
  EXPECT_TRUE(base_dict.GetMinimalRole("base.command2", &role, nullptr));
  EXPECT_EQ(UserRole::kUser, role);
  EXPECT_TRUE(base_dict.GetMinimalRole("base.command3", &role, nullptr));
  EXPECT_EQ(UserRole::kManager, role);
  EXPECT_TRUE(base_dict.GetMinimalRole("base.command4", &role, nullptr));
  EXPECT_EQ(UserRole::kOwner, role);
  EXPECT_FALSE(base_dict.GetMinimalRole("base.command5", &role, nullptr));
}

}  // namespace weave
