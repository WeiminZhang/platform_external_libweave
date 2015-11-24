// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/states/state_package.h"

#include <memory>
#include <string>

#include <base/values.h>
#include <gtest/gtest.h>

#include "src/commands/schema_constants.h"
#include "src/commands/unittest_utils.h"
#include "src/states/error_codes.h"

namespace weave {

using test::CreateDictionaryValue;

class StatePackageTestHelper {
 public:
  // Returns the state property definitions (types/constraints/etc).
  static const base::DictionaryValue& GetTypes(const StatePackage& package) {
    return package.types_;
  }
  // Returns the all state property values in this package.
  static const base::DictionaryValue& GetValues(const StatePackage& package) {
    return package.values_;
  }
};

namespace {
std::unique_ptr<base::DictionaryValue> GetTestSchema() {
  return CreateDictionaryValue(R"({
    'color': {
      'type': 'string'
    },
    'direction': {
      'additionalProperties': false,
      'properties': {
        'altitude': {
          'maximum': 90.0,
          'type': 'number'
        },
        'azimuth': {
          'type': 'number'
        }
      },
      'type': 'object',
      'required': [ 'azimuth' ]
    },
    'iso': {
      'enum': [50, 100, 200, 400],
      'type': 'integer'
    },
    'light': {
      'type': 'boolean'
    }
  })");
}

std::unique_ptr<base::DictionaryValue> GetTestValues() {
  return CreateDictionaryValue(R"({
      'light': true,
      'color': 'white',
      'direction': {'azimuth':57.2957795, 'altitude':89.9},
      'iso': 200
  })");
}

inline const base::DictionaryValue& GetTypes(const StatePackage& package) {
  return StatePackageTestHelper::GetTypes(package);
}
// Returns the all state property values in this package.
inline const base::DictionaryValue& GetValues(const StatePackage& package) {
  return StatePackageTestHelper::GetValues(package);
}

}  // anonymous namespace

class StatePackageTest : public ::testing::Test {
 public:
  void SetUp() override {
    package_.reset(new StatePackage("test"));
    ASSERT_TRUE(package_->AddSchemaFromJson(GetTestSchema().get(), nullptr));
    ASSERT_TRUE(package_->AddValuesFromJson(GetTestValues().get(), nullptr));
  }
  void TearDown() override { package_.reset(); }
  std::unique_ptr<StatePackage> package_;
};

TEST(StatePackage, Empty) {
  StatePackage package("test");
  EXPECT_EQ("test", package.GetName());
  EXPECT_TRUE(GetTypes(package).empty());
  EXPECT_TRUE(GetValues(package).empty());
}

TEST(StatePackage, AddSchemaFromJson_OnEmpty) {
  StatePackage package("test");
  ASSERT_TRUE(package.AddSchemaFromJson(GetTestSchema().get(), nullptr));
  EXPECT_EQ(4, GetTypes(package).size());
  EXPECT_EQ(0, GetValues(package).size());

  auto expected = R"({
    'color': {
      'type': 'string'
    },
    'direction': {
      'additionalProperties': false,
      'properties': {
        'altitude': {
          'maximum': 90.0,
          'type': 'number'
        },
        'azimuth': {
          'type': 'number'
        }
      },
      'type': 'object',
      'required': [ 'azimuth' ]
    },
    'iso': {
      'enum': [50, 100, 200, 400],
      'type': 'integer'
    },
    'light': {
      'type': 'boolean'
    }
  })";
  EXPECT_JSON_EQ(expected, GetTypes(package));

  EXPECT_JSON_EQ("{}", *package.GetValuesAsJson());
}

TEST(StatePackage, AddValuesFromJson_OnEmpty) {
  StatePackage package("test");
  ASSERT_TRUE(package.AddSchemaFromJson(GetTestSchema().get(), nullptr));
  ASSERT_TRUE(package.AddValuesFromJson(GetTestValues().get(), nullptr));
  EXPECT_EQ(4, GetValues(package).size());
  auto expected = R"({
    'color': 'white',
    'direction': {
      'altitude': 89.9,
      'azimuth': 57.2957795
    },
    'iso': 200,
    'light': true
  })";
  EXPECT_JSON_EQ(expected, *package.GetValuesAsJson());
}

TEST_F(StatePackageTest, AddSchemaFromJson_AddMore) {
  auto dict = CreateDictionaryValue(R"({'brightness':{
      'enum': ['low', 'medium', 'high'],
      'type': 'string'
    }})");
  ASSERT_TRUE(package_->AddSchemaFromJson(dict.get(), nullptr));
  EXPECT_EQ(5, GetTypes(*package_).size());
  EXPECT_EQ(4, GetValues(*package_).size());
  auto expected = R"({
    'brightness': {
      'enum': ['low', 'medium', 'high'],
      'type': 'string'
    },
    'color': {
      'type': 'string'
    },
    'direction': {
      'additionalProperties': false,
      'properties': {
        'altitude': {
          'maximum': 90.0,
          'type': 'number'
        },
        'azimuth': {
          'type': 'number'
        }
      },
      'type': 'object',
      'required': [ 'azimuth' ]
    },
    'iso': {
      'enum': [50, 100, 200, 400],
      'type': 'integer'
    },
    'light': {
      'type': 'boolean'
    }
  })";
  EXPECT_JSON_EQ(expected, GetTypes(*package_));

  expected = R"({
    'color': 'white',
    'direction': {
      'altitude': 89.9,
      'azimuth': 57.2957795
    },
    'iso': 200,
    'light': true
  })";
  EXPECT_JSON_EQ(expected, *package_->GetValuesAsJson());
}

TEST_F(StatePackageTest, AddValuesFromJson_AddMore) {
  auto dict = CreateDictionaryValue(R"({'brightness':{
      'enum': ['low', 'medium', 'high'],
      'type': 'string'
    }})");
  ASSERT_TRUE(package_->AddSchemaFromJson(dict.get(), nullptr));
  dict = CreateDictionaryValue("{'brightness':'medium'}");
  ASSERT_TRUE(package_->AddValuesFromJson(dict.get(), nullptr));
  EXPECT_EQ(5, GetValues(*package_).size());
  auto expected = R"({
    'brightness': 'medium',
    'color': 'white',
    'direction': {
      'altitude': 89.9,
      'azimuth': 57.2957795
    },
    'iso': 200,
    'light': true
  })";
  EXPECT_JSON_EQ(expected, *package_->GetValuesAsJson());
}

TEST_F(StatePackageTest, AddSchemaFromJson_Error_Redefined) {
  auto dict = CreateDictionaryValue(R"({'color':
    {'type':'string', 'enum':['white', 'blue', 'red']}})");
  ErrorPtr error;
  EXPECT_FALSE(package_->AddSchemaFromJson(dict.get(), &error));
  EXPECT_EQ(errors::state::kDomain, error->GetDomain());
  EXPECT_EQ(errors::state::kPropertyRedefinition, error->GetCode());
}

TEST_F(StatePackageTest, AddValuesFromJson_Error_Undefined) {
  auto dict = CreateDictionaryValue("{'brightness':'medium'}");
  EXPECT_TRUE(package_->AddValuesFromJson(dict.get(), nullptr));
}

TEST_F(StatePackageTest, GetPropertyValue) {
  EXPECT_JSON_EQ("'white'", *package_->GetPropertyValue("color", nullptr));
  EXPECT_JSON_EQ("true", *package_->GetPropertyValue("light", nullptr));
  EXPECT_JSON_EQ("200", *package_->GetPropertyValue("iso", nullptr));
  EXPECT_JSON_EQ("{'altitude': 89.9, 'azimuth': 57.2957795}",
                 *package_->GetPropertyValue("direction", nullptr));
}

TEST_F(StatePackageTest, GetPropertyValue_Unknown) {
  ErrorPtr error;
  EXPECT_EQ(nullptr, package_->GetPropertyValue("volume", &error));
  EXPECT_EQ(errors::state::kDomain, error->GetDomain());
  EXPECT_EQ(errors::state::kPropertyNotDefined, error->GetCode());
}

TEST_F(StatePackageTest, SetPropertyValue_Simple) {
  EXPECT_TRUE(
      package_->SetPropertyValue("color", base::StringValue{"blue"}, nullptr));
  EXPECT_JSON_EQ("'blue'", *package_->GetPropertyValue("color", nullptr));
  EXPECT_TRUE(package_->SetPropertyValue("light", base::FundamentalValue{false},
                                         nullptr));
  bool light = false;
  ASSERT_TRUE(
      package_->GetPropertyValue("light", nullptr)->GetAsBoolean(&light));
  EXPECT_FALSE(light);
  EXPECT_TRUE(
      package_->SetPropertyValue("iso", base::FundamentalValue{400}, nullptr));
  EXPECT_JSON_EQ("400", *package_->GetPropertyValue("iso", nullptr));
}

TEST_F(StatePackageTest, SetPropertyValue_Object) {
  EXPECT_TRUE(package_->SetPropertyValue(
      "direction",
      *CreateDictionaryValue("{'altitude': 45.0, 'azimuth': 15.0}"), nullptr));

  auto expected = R"({
    'color': 'white',
    'direction': {
      'altitude': 45.0,
      'azimuth': 15.0
    },
    'iso': 200,
    'light': true
  })";
  EXPECT_JSON_EQ(expected, *package_->GetValuesAsJson());
}

}  // namespace weave
