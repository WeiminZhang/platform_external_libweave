// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/states/state_manager.h"

#include <cstdlib>  // for abs().
#include <vector>

#include <base/bind.h>
#include <base/values.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <weave/provider/test/mock_config_store.h>

#include "src/commands/schema_constants.h"
#include "src/commands/unittest_utils.h"
#include "src/states/error_codes.h"
#include "src/states/mock_state_change_queue_interface.h"

namespace weave {

using testing::_;
using testing::Return;
using test::CreateDictionaryValue;

namespace {

const char kBaseDefinition[] = R"({
  "base": {
    "manufacturer":"string",
    "serialNumber":"string"
  },
  "device": {
    "state_property":"string"
  }
})";

std::unique_ptr<base::DictionaryValue> GetTestSchema() {
  return CreateDictionaryValue(kBaseDefinition);
}

const char kBaseDefaults[] = R"({
  "base": {
    "manufacturer":"Test Factory",
    "serialNumber":"Test Model"
  }
})";

std::unique_ptr<base::DictionaryValue> GetTestValues() {
  return CreateDictionaryValue(kBaseDefaults);
}

}  // anonymous namespace

class StateManagerTest : public ::testing::Test {
 public:
  void SetUp() override {
    // Initial expectations.
    EXPECT_CALL(mock_state_change_queue_, IsEmpty()).Times(0);
    EXPECT_CALL(mock_state_change_queue_, NotifyPropertiesUpdated(_, _))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(mock_state_change_queue_, GetAndClearRecordedStateChanges())
        .Times(0);
    mgr_.reset(new StateManager(&mock_state_change_queue_));

    EXPECT_CALL(*this, OnStateChanged()).Times(2);
    mgr_->AddChangedCallback(
        base::Bind(&StateManagerTest::OnStateChanged, base::Unretained(this)));

    LoadStateDefinition(GetTestSchema().get(), nullptr);
    ASSERT_TRUE(mgr_->SetProperties(*GetTestValues().get(), nullptr));
  }
  void TearDown() override { mgr_.reset(); }

  void LoadStateDefinition(const base::DictionaryValue* json,
                           ErrorPtr* error) {
    ASSERT_TRUE(mgr_->LoadStateDefinition(*json, error));
  }

  bool SetPropertyValue(const std::string& name,
                        const base::Value& value,
                        ErrorPtr* error) {
    return mgr_->SetPropertyValue(name, value, timestamp_, error);
  }

  MOCK_CONST_METHOD0(OnStateChanged, void());

  base::Time timestamp_{base::Time::Now()};
  std::unique_ptr<StateManager> mgr_;
  testing::StrictMock<MockStateChangeQueueInterface> mock_state_change_queue_;
};

TEST(StateManager, Empty) {
  testing::StrictMock<MockStateChangeQueueInterface> mock_state_change_queue;
  StateManager manager(&mock_state_change_queue);
}

TEST_F(StateManagerTest, Initialized) {
  auto expected = R"({
    'base': {
      'manufacturer': 'Test Factory',
      'serialNumber': 'Test Model'
    },
    'device': {
      'state_property': ''
    }
  })";
  EXPECT_JSON_EQ(expected, *mgr_->GetState());
}

TEST_F(StateManagerTest, LoadStateDefinition) {
  auto dict = CreateDictionaryValue(R"({
    'power': {
      'battery_level':'integer'
    }
  })");
  LoadStateDefinition(dict.get(), nullptr);

  auto expected = R"({
    'base': {
      'manufacturer': 'Test Factory',
      'serialNumber': 'Test Model'
    },
    'power': {
      'battery_level': 0
    },
    'device': {
      'state_property': ''
    }
  })";
  EXPECT_JSON_EQ(expected, *mgr_->GetState());
}

TEST_F(StateManagerTest, Startup) {
  StateManager manager(&mock_state_change_queue_);

  auto state_definition = R"({
    "base": {
      "firmwareVersion": "string",
      "localDiscoveryEnabled": "boolean",
      "localAnonymousAccessMaxRole": [ "none", "viewer", "user" ],
      "localPairingEnabled": "boolean"
    },
    "power": {"battery_level":"integer"}
  })";
  ASSERT_TRUE(manager.LoadStateDefinitionFromJson(state_definition, nullptr));

  auto state_values = R"({
    "base": {
      "firmwareVersion": "unknown",
      "localDiscoveryEnabled": false,
      "localAnonymousAccessMaxRole": "none",
      "localPairingEnabled": false
    },
    "power": {"battery_level":44}
  })";
  ASSERT_TRUE(manager.SetPropertiesFromJson(state_values, nullptr));

  auto expected = R"({
    'base': {
      'firmwareVersion': 'unknown',
      'localAnonymousAccessMaxRole': 'none',
      'localDiscoveryEnabled': false,
      'localPairingEnabled': false
    },
    'power': {
      'battery_level': 44
    }
  })";
  EXPECT_JSON_EQ(expected, *manager.GetState());
}

TEST_F(StateManagerTest, SetPropertyValue) {
  ValueMap expected_prop_set{
      {"device.state_property", test::make_string_prop_value("Test Value")},
  };
  EXPECT_CALL(mock_state_change_queue_,
              NotifyPropertiesUpdated(timestamp_, expected_prop_set))
      .WillOnce(Return(true));
  ASSERT_TRUE(SetPropertyValue("device.state_property",
                               base::StringValue{"Test Value"}, nullptr));
  auto expected = R"({
    'base': {
      'manufacturer': 'Test Factory',
      'serialNumber': 'Test Model'
    },
    'device': {
      'state_property': 'Test Value'
    }
  })";
  EXPECT_JSON_EQ(expected, *mgr_->GetState());
}

TEST_F(StateManagerTest, SetPropertyValue_Error_NoName) {
  ErrorPtr error;
  ASSERT_FALSE(SetPropertyValue("", base::FundamentalValue{0}, &error));
  EXPECT_EQ(errors::state::kDomain, error->GetDomain());
  EXPECT_EQ(errors::state::kPropertyNameMissing, error->GetCode());
}

TEST_F(StateManagerTest, SetPropertyValue_Error_NoPackage) {
  ErrorPtr error;
  ASSERT_FALSE(
      SetPropertyValue("state_property", base::FundamentalValue{0}, &error));
  EXPECT_EQ(errors::state::kDomain, error->GetDomain());
  EXPECT_EQ(errors::state::kPackageNameMissing, error->GetCode());
}

TEST_F(StateManagerTest, SetPropertyValue_Error_UnknownPackage) {
  ErrorPtr error;
  ASSERT_FALSE(
      SetPropertyValue("power.level", base::FundamentalValue{0}, &error));
  EXPECT_EQ(errors::state::kDomain, error->GetDomain());
  EXPECT_EQ(errors::state::kPropertyNotDefined, error->GetCode());
}

TEST_F(StateManagerTest, SetPropertyValue_Error_UnknownProperty) {
  ErrorPtr error;
  ASSERT_FALSE(
      SetPropertyValue("base.level", base::FundamentalValue{0}, &error));
  EXPECT_EQ(errors::state::kDomain, error->GetDomain());
  EXPECT_EQ(errors::state::kPropertyNotDefined, error->GetCode());
}

TEST_F(StateManagerTest, GetAndClearRecordedStateChanges) {
  EXPECT_CALL(mock_state_change_queue_, NotifyPropertiesUpdated(timestamp_, _))
      .WillOnce(Return(true));
  ASSERT_TRUE(SetPropertyValue("device.state_property",
                               base::StringValue{"Test Value"}, nullptr));
  std::vector<StateChange> expected_val;
  expected_val.emplace_back(
      timestamp_, ValueMap{{"device.state_property",
                            test::make_string_prop_value("Test Value")}});
  EXPECT_CALL(mock_state_change_queue_, GetAndClearRecordedStateChanges())
      .WillOnce(Return(expected_val));
  EXPECT_CALL(mock_state_change_queue_, GetLastStateChangeId())
      .WillOnce(Return(0));
  auto changes = mgr_->GetAndClearRecordedStateChanges();
  ASSERT_EQ(1, changes.second.size());
  EXPECT_EQ(expected_val.back().timestamp, changes.second.back().timestamp);
  EXPECT_EQ(expected_val.back().changed_properties,
            changes.second.back().changed_properties);
}

TEST_F(StateManagerTest, SetProperties) {
  ValueMap expected_prop_set{
      {"base.manufacturer", test::make_string_prop_value("No Name")},
  };
  EXPECT_CALL(mock_state_change_queue_,
              NotifyPropertiesUpdated(_, expected_prop_set))
      .WillOnce(Return(true));

  EXPECT_CALL(*this, OnStateChanged()).Times(1);
  ASSERT_TRUE(mgr_->SetProperties(
      *CreateDictionaryValue("{'base':{'manufacturer':'No Name'}}"), nullptr));

  auto expected = R"({
    'base': {
      'manufacturer': 'No Name',
      'serialNumber': 'Test Model'
    },
    'device': {
      'state_property': ''
    }
  })";
  EXPECT_JSON_EQ(expected, *mgr_->GetState());
}

TEST_F(StateManagerTest, GetProperty) {
  EXPECT_JSON_EQ("'Test Model'", *mgr_->GetProperty("base.serialNumber"));
  EXPECT_JSON_EQ("''", *mgr_->GetProperty("device.state_property"));
  EXPECT_EQ(nullptr, mgr_->GetProperty("device.unknown"));
  EXPECT_EQ(nullptr, mgr_->GetProperty("unknown.state_property"));
}

}  // namespace weave
