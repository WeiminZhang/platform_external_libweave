// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/commands/command_definition.h"

#include <gtest/gtest.h>
#include <weave/test/unittest_utils.h>

namespace weave {

using test::CreateDictionaryValue;

TEST(CommandDefinition, DefaultRole) {
  auto params = CreateDictionaryValue(R"({
    'parameters': {
      'height': 'integer',
      'jumpType': ['_withAirFlip', '_withSpin', '_withKick']
    },
    'progress': {'progress': 'integer'},
    'results': {'testResult': 'integer'}
  })");
  auto def = CommandDefinition::FromJson(*params, nullptr);
  EXPECT_EQ(UserRole::kUser, def->GetMinimalRole());
}

TEST(CommandDefinition, SpecifiedRole) {
  auto params = CreateDictionaryValue(R"({
    'parameters': {},
    'progress': {},
    'results': {},
    'minimalRole': 'owner'
  })");
  auto def = CommandDefinition::FromJson(*params, nullptr);
  EXPECT_EQ(UserRole::kOwner, def->GetMinimalRole());
}

TEST(CommandDefinition, IncorrectRole) {
  auto params = CreateDictionaryValue(R"({
    'parameters': {},
    'progress': {},
    'results': {},
    'minimalRole': 'foo'
  })");
  ErrorPtr error;
  auto def = CommandDefinition::FromJson(*params, &error);
  EXPECT_EQ(nullptr, def.get());
  EXPECT_EQ("invalid_parameter_value", error->GetCode());
}

}  // namespace weave
