// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/access_api_handler.h"

#include <gtest/gtest.h>
#include <weave/provider/test/fake_task_runner.h>
#include <weave/test/mock_device.h>
#include <weave/test/unittest_utils.h>

#include "src/access_revocation_manager.h"
#include "src/component_manager_impl.h"
#include "src/data_encoding.h"
#include "src/test/mock_access_revocation_manager.h"

using testing::_;
using testing::AnyOf;
using testing::Invoke;
using testing::Return;
using testing::StrictMock;
using testing::WithArgs;

namespace weave {

class AccessApiHandlerTest : public ::testing::Test {
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
                AddCommandHandler(_, AnyOf("_accessRevocationList.add",
                                           "_accessRevocationList.list"),
                                  _))
        .WillRepeatedly(
            Invoke(&component_manager_, &ComponentManager::AddCommandHandler));

    EXPECT_CALL(access_manager_, GetSize()).WillRepeatedly(Return(0));

    EXPECT_CALL(access_manager_, GetCapacity()).WillRepeatedly(Return(10));

    handler_.reset(new AccessApiHandler{&device_, &access_manager_});
  }

  const base::DictionaryValue& AddCommand(const std::string& command) {
    std::string id;
    auto command_instance = component_manager_.ParseCommandInstance(
        *test::CreateDictionaryValue(command.c_str()), Command::Origin::kLocal,
        UserRole::kOwner, &id, nullptr);
    EXPECT_NE(nullptr, command_instance.get());
    component_manager_.AddCommand(std::move(command_instance));
    EXPECT_EQ(Command::State::kDone,
              component_manager_.FindCommand(id)->GetState());
    return component_manager_.FindCommand(id)->GetResults();
  }

  std::unique_ptr<base::DictionaryValue> GetState() {
    std::string path =
        component_manager_.FindComponentWithTrait("_accessRevocationList");
    EXPECT_FALSE(path.empty());
    const auto* component = component_manager_.FindComponent(path, nullptr);
    EXPECT_TRUE(component);
    const base::DictionaryValue* state = nullptr;
    EXPECT_TRUE(
        component->GetDictionary("state._accessRevocationList", &state));
    return std::unique_ptr<base::DictionaryValue>{state->DeepCopy()};
  }

  StrictMock<provider::test::FakeTaskRunner> task_runner_;
  ComponentManagerImpl component_manager_{&task_runner_};
  StrictMock<test::MockDevice> device_;
  StrictMock<test::MockAccessRevocationManager> access_manager_;
  std::unique_ptr<AccessApiHandler> handler_;
};

TEST_F(AccessApiHandlerTest, Initialization) {
  const base::DictionaryValue* trait = nullptr;
  ASSERT_TRUE(component_manager_.GetTraits().GetDictionary(
      "_accessRevocationList", &trait));

  auto expected = R"({
    "commands": {
      "add": {
        "minimalRole": "owner",
        "parameters": {
          "userId": {
            "type": "string"
          },
          "applicationId": {
            "type": "string"
          },
          "revocationTimestamp": {
            "type": "integer"
          },
          "expirationTime": {
            "type": "integer"
          }
        }
      },
      "list": {
        "minimalRole": "owner",
        "parameters": {},
        "results": {
          "revocationList": {
            "type": "array",
            "items": {
              "type": "object",
              "properties": {
                "userId": {
                  "type": "string"
                },
                "applicationId": {
                  "type": "string"
                },
                "revocationTimestamp": {
                  "type": "integer"
                },
                "expirationTime": {
                  "type": "integer"
                }
              },
              "additionalProperties": false
            }
          }
        }
      }
    },
    "state": {
      "capacity": {
        "type": "integer",
        "isRequired": true
      }
    }
  })";
  EXPECT_JSON_EQ(expected, *trait);

  expected = R"({
    "capacity": 10
  })";
  EXPECT_JSON_EQ(expected, *GetState());
}

TEST_F(AccessApiHandlerTest, Revoke) {
  EXPECT_CALL(
      access_manager_,
      Block(AccessRevocationManager::Entry{std::vector<uint8_t>{1, 2, 3},
                                           std::vector<uint8_t>{3, 4, 5},
                                           base::Time::FromTimeT(946686034),
                                           base::Time::FromTimeT(946692690)},
            _))
      .WillOnce(WithArgs<1>(
          Invoke([](const DoneCallback& callback) { callback.Run(nullptr); })));
  EXPECT_CALL(access_manager_, GetSize()).WillRepeatedly(Return(1));

  AddCommand(R"({
    'name' : '_accessRevocationList.add',
    'component': 'accessControl',
    'parameters': {
      'userId': 'AQID',
      'applicationId': 'AwQF',
      'expirationTime': 7890,
      'revocationTimestamp': 1234
    }
  })");

  auto expected = R"({
    "capacity": 10
  })";
  EXPECT_JSON_EQ(expected, *GetState());
}

TEST_F(AccessApiHandlerTest, List) {
  std::vector<AccessRevocationManager::Entry> entries{
      {{11, 12, 13},
       {21, 22, 23},
       base::Time::FromTimeT(1310000000),
       base::Time::FromTimeT(1410000000)},
      {{31, 32, 33},
       {41, 42, 43},
       base::Time::FromTimeT(1300000000),
       base::Time::FromTimeT(1420000000)},
  };
  EXPECT_CALL(access_manager_, GetEntries()).WillOnce(Return(entries));
  EXPECT_CALL(access_manager_, GetSize()).WillRepeatedly(Return(4));

  auto expected = R"({
    "revocationList": [ {
      "applicationId": "FRYX",
      "userId": "CwwN"
    }, {
       "applicationId": "KSor",
       "userId": "HyAh"
    } ]
  })";

  const auto& results = AddCommand(R"({
    'name' : '_accessRevocationList.list',
    'component': 'accessControl',
    'parameters': {
    }
  })");

  EXPECT_JSON_EQ(expected, results);
}
}  // namespace weave
