// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/base_api_handler.h"

#include <base/strings/string_number_conversions.h>
#include <base/time/default_clock.h>
#include <base/values.h>
#include <gtest/gtest.h>
#include <weave/provider/test/fake_task_runner.h>
#include <weave/provider/test/mock_config_store.h>
#include <weave/provider/test/mock_http_client.h>
#include <weave/test/mock_device.h>
#include <weave/test/unittest_utils.h>

#include "src/component_manager_impl.h"
#include "src/config.h"
#include "src/device_registration_info.h"

using testing::_;
using testing::AnyOf;
using testing::Eq;
using testing::Invoke;
using testing::Return;
using testing::ReturnRef;
using testing::StrictMock;

namespace weave {

class BaseApiHandlerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    EXPECT_CALL(device_, AddTraitDefinitionsFromJson(_))
        .WillRepeatedly(Invoke([this](const std::string& json) {
          EXPECT_TRUE(component_manager_.LoadTraits(json, nullptr));
        }));
    EXPECT_CALL(device_, SetStateProperties(_, _, _))
        .WillRepeatedly(
            Invoke(&component_manager_, &ComponentManager::SetStateProperties));
    EXPECT_CALL(device_, SetStateProperty(_, _, _, _))
        .WillRepeatedly(
            Invoke(&component_manager_, &ComponentManager::SetStateProperty));
    EXPECT_CALL(device_, AddComponent(_, _, _))
        .WillRepeatedly(Invoke([this](const std::string& name,
                                      const std::vector<std::string>& traits,
                                      ErrorPtr* error) {
          return component_manager_.AddComponent("", name, traits, error);
        }));

    EXPECT_CALL(device_,
                AddCommandHandler(_,
                                  AnyOf("device.setConfig", "privet.setConfig"),
                                  _))
        .WillRepeatedly(
            Invoke(&component_manager_, &ComponentManager::AddCommandHandler));

    dev_reg_.reset(new DeviceRegistrationInfo(&config_, &component_manager_,
                                              nullptr, &http_client_, nullptr,
                                              nullptr));

    EXPECT_CALL(device_, GetSettings())
        .WillRepeatedly(ReturnRef(dev_reg_->GetSettings()));

    handler_.reset(new BaseApiHandler{dev_reg_.get(), &device_});
  }

  void AddCommand(const std::string& command) {
    std::string id;
    auto command_instance = component_manager_.ParseCommandInstance(
        *test::CreateDictionaryValue(command.c_str()), Command::Origin::kLocal,
        UserRole::kOwner, &id, nullptr);
    ASSERT_NE(nullptr, command_instance.get());
    component_manager_.AddCommand(std::move(command_instance));
    EXPECT_EQ(Command::State::kDone,
              component_manager_.FindCommand(id)->GetState());
  }

  std::unique_ptr<base::DictionaryValue> GetDeviceState() {
    std::unique_ptr<base::DictionaryValue> state;
    std::string path = component_manager_.FindComponentWithTrait("device");
    EXPECT_FALSE(path.empty());
    const auto* component = component_manager_.FindComponent(path, nullptr);
    CHECK(component);
    const base::DictionaryValue* base_state = nullptr;
    if (component->GetDictionary("state.device", &base_state))
      state = base_state->CreateDeepCopy();
    else
      state.reset(new base::DictionaryValue);
    return state;
  }

  std::unique_ptr<base::DictionaryValue> GetPrivetState() {
    std::unique_ptr<base::DictionaryValue> state;
    std::string path = component_manager_.FindComponentWithTrait("privet");
    EXPECT_FALSE(path.empty());
    const auto* component = component_manager_.FindComponent(path, nullptr);
    CHECK(component);
    const base::DictionaryValue* base_state = nullptr;
    if (component->GetDictionary("state.privet", &base_state))
      state = base_state->CreateDeepCopy();
    else
      state.reset(new base::DictionaryValue);
    return state;
  }

  provider::test::MockConfigStore config_store_;
  Config config_{&config_store_};
  StrictMock<provider::test::MockHttpClient> http_client_;
  std::unique_ptr<DeviceRegistrationInfo> dev_reg_;
  StrictMock<provider::test::FakeTaskRunner> task_runner_;
  ComponentManagerImpl component_manager_{&task_runner_};
  std::unique_ptr<BaseApiHandler> handler_;
  StrictMock<test::MockDevice> device_;
};

TEST_F(BaseApiHandlerTest, Initialization) {
  const base::DictionaryValue* trait = nullptr;
  ASSERT_TRUE(component_manager_.GetTraits().GetDictionary("device", &trait));

  auto expected1 = R"({
    "commands": {
      "setConfig": {
        "minimalRole": "user",
        "parameters": {
          "name": {
            "type": "string"
          },
          "description": {
            "type": "string"
          },
          "location": {
            "type": "string"
          }
        }
      }
    },
    "state": {
      "name": {
        "isRequired": true,
        "type": "string"
      },
      "description": {
        "isRequired": true,
        "type": "string"
      },
      "location": {
        "type": "string"
      },
      "hardwareId": {
        "isRequired": true,
        "type": "string"
      },
      "serialNumber": {
        "isRequired": true,
        "type": "string"
      },
      "firmwareVersion": {
        "isRequired": true,
        "type": "string"
      }
    }
  })";
  EXPECT_JSON_EQ(expected1, *trait);

  ASSERT_TRUE(component_manager_.GetTraits().GetDictionary("privet", &trait));

  auto expected2 = R"({
    "commands": {
      "setConfig": {
        "minimalRole": "manager",
        "parameters": {
          "isLocalAccessEnabled": {
            "type": "boolean"
          },
          "maxRoleForAnonymousAccess": {
            "type": "string",
            "enum": [ "none", "viewer", "user", "manager" ]
          }
        }
      }
    },
    "state": {
      "apiVersion": {
        "isRequired": true,
        "type": "string"
      },
      "isLocalAccessEnabled": {
        "isRequired": true,
        "type": "boolean"
      },
      "maxRoleForAnonymousAccess": {
        "isRequired": true,
        "type": "string",
        "enum": [ "none", "viewer", "user", "manager" ]
      }
    }
  })";
  EXPECT_JSON_EQ(expected2, *trait);
}

TEST_F(BaseApiHandlerTest, PrivetSetConfig) {
  const Settings& settings = dev_reg_->GetSettings();

  AddCommand(R"({
    'name' : 'privet.setConfig',
    'component': 'device',
    'parameters': {
      'isLocalAccessEnabled': false,
      'maxRoleForAnonymousAccess': 'none'
    }
  })");
  EXPECT_EQ(AuthScope::kNone, settings.local_anonymous_access_role);
  EXPECT_FALSE(settings.local_access_enabled);

  auto expected = R"({
    'apiVersion': '3',
    'maxRoleForAnonymousAccess': 'none',
    'isLocalAccessEnabled': false
  })";
  EXPECT_JSON_EQ(expected, *GetPrivetState());

  AddCommand(R"({
    'name' : 'privet.setConfig',
    'component': 'device',
    'parameters': {
      'maxRoleForAnonymousAccess': 'user',
      'isLocalAccessEnabled': true
    }
  })");
  EXPECT_EQ(AuthScope::kUser, settings.local_anonymous_access_role);
  EXPECT_TRUE(settings.local_access_enabled);
  expected = R"({
    'apiVersion': '3',
    'maxRoleForAnonymousAccess': 'user',
    'isLocalAccessEnabled': true
  })";
  EXPECT_JSON_EQ(expected, *GetPrivetState());

  {
    Config::Transaction change{dev_reg_->GetMutableConfig()};
    change.set_local_anonymous_access_role(AuthScope::kViewer);
  }
  expected = R"({
    'apiVersion': '3',
    'maxRoleForAnonymousAccess': 'viewer',
    'isLocalAccessEnabled': true
  })";
  EXPECT_JSON_EQ(expected, *GetPrivetState());
}

TEST_F(BaseApiHandlerTest, DeviceSetConfig) {
  AddCommand(R"({
    'name' : 'device.setConfig',
    'component': 'device',
    'parameters': {
      'name': 'testName',
      'description': 'testDescription',
      'location': 'testLocation'
    }
  })");

  const Settings& config = dev_reg_->GetSettings();
  EXPECT_EQ("testName", config.name);
  EXPECT_EQ("testDescription", config.description);
  EXPECT_EQ("testLocation", config.location);
  auto expected = R"({
    'name': 'testName',
    'description': 'testDescription',
    'location': 'testLocation',
    'hardwareId': 'TEST_DEVICE_ID',
    'serialNumber': 'TEST_SERIAL_NUMBER',
    'firmwareVersion': 'TEST_FIRMWARE'
  })";
  EXPECT_JSON_EQ(expected, *GetDeviceState());

  AddCommand(R"({
    'name' : 'device.setConfig',
    'component': 'device',
    'parameters': {
      'location': 'newLocation'
    }
  })");

  EXPECT_EQ("testName", config.name);
  EXPECT_EQ("testDescription", config.description);
  EXPECT_EQ("newLocation", config.location);

  expected = R"({
    'name': 'testName',
    'description': 'testDescription',
    'location': 'newLocation',
    'hardwareId': 'TEST_DEVICE_ID',
    'serialNumber': 'TEST_SERIAL_NUMBER',
    'firmwareVersion': 'TEST_FIRMWARE'
  })";
  EXPECT_JSON_EQ(expected, *GetDeviceState());
}

}  // namespace weave
