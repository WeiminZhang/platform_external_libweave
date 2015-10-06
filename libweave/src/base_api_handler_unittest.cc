// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/base_api_handler.h"

#include <base/strings/string_number_conversions.h>
#include <base/values.h>
#include <gtest/gtest.h>
#include <weave/provider/test/mock_config_store.h>
#include <weave/provider/test/mock_http_client.h>
#include <weave/test/mock_device.h>

#include "src/commands/command_manager.h"
#include "src/commands/unittest_utils.h"
#include "src/config.h"
#include "src/device_registration_info.h"
#include "src/states/mock_state_change_queue_interface.h"
#include "src/states/state_manager.h"

using testing::_;
using testing::Invoke;
using testing::Return;
using testing::StrictMock;

namespace weave {

class BaseApiHandlerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    EXPECT_CALL(mock_state_change_queue_, NotifyPropertiesUpdated(_, _))
        .WillRepeatedly(Return(true));

    command_manager_ = std::make_shared<CommandManager>();
    state_manager_ = std::make_shared<StateManager>(&mock_state_change_queue_);

    EXPECT_CALL(device_, SetStateProperties(_, _))
        .WillRepeatedly(
            Invoke(state_manager_.get(), &StateManager::SetProperties));
    EXPECT_CALL(device_, AddCommandAddedCallback(_))
        .WillRepeatedly(Invoke(command_manager_.get(),
                               &CommandManager::AddCommandAddedCallback));

    auto state_definition = test::CreateDictionaryValue(R"({
      'base': {
        'firmwareVersion': 'string',
        'localDiscoveryEnabled': 'boolean',
        'localAnonymousAccessMaxRole': [ 'none', 'viewer', 'user' ],
        'localPairingEnabled': 'boolean',
        'network': {
          'properties': {
            'name': 'string'
          }
        }
      }
    })");
    auto state_defaults = test::CreateDictionaryValue(R"({
      'base': {
        'firmwareVersion': '123123',
        'localDiscoveryEnabled': false,
        'localAnonymousAccessMaxRole': 'none',
        'localPairingEnabled': false
      }
    })");
    ASSERT_TRUE(
        state_manager_->LoadStateDefinition(*state_definition, nullptr));
    ASSERT_TRUE(state_manager_->LoadStateDefaults(*state_defaults, nullptr));
    std::unique_ptr<Config> config{new Config{&config_store_}};
    config->Load();
    dev_reg_.reset(new DeviceRegistrationInfo(command_manager_, state_manager_,
                                              std::move(config), nullptr,
                                              &http_client_, nullptr));
    handler_.reset(new BaseApiHandler{dev_reg_.get(), &device_});
  }

  void LoadCommands(const std::string& command_definitions) {
    auto json = test::CreateDictionaryValue(command_definitions.c_str());
    EXPECT_TRUE(command_manager_->LoadBaseCommands(*json, nullptr));
    EXPECT_TRUE(command_manager_->LoadCommands(*json, nullptr));
  }

  void AddCommand(const std::string& command) {
    auto command_instance = CommandInstance::FromJson(
        test::CreateDictionaryValue(command.c_str()).get(),
        CommandOrigin::kLocal, command_manager_->GetCommandDictionary(),
        nullptr, nullptr);
    EXPECT_TRUE(!!command_instance);

    std::string id{base::IntToString(++command_id_)};
    command_instance->SetID(id);
    command_manager_->AddCommand(std::move(command_instance));
    EXPECT_EQ(CommandStatus::kDone,
              command_manager_->FindCommand(id)->GetStatus());
  }

  provider::test::MockConfigStore config_store_;
  StrictMock<provider::test::MockHttpClient> http_client_;
  std::unique_ptr<DeviceRegistrationInfo> dev_reg_;
  std::shared_ptr<CommandManager> command_manager_;
  testing::StrictMock<MockStateChangeQueueInterface> mock_state_change_queue_;
  std::shared_ptr<StateManager> state_manager_;
  std::unique_ptr<BaseApiHandler> handler_;
  StrictMock<test::MockDevice> device_;
  int command_id_{0};
};

TEST_F(BaseApiHandlerTest, UpdateBaseConfiguration) {
  LoadCommands(R"({
    'base': {
      'updateBaseConfiguration': {
        'parameters': {
          'localDiscoveryEnabled': 'boolean',
          'localAnonymousAccessMaxRole': [ 'none', 'viewer', 'user' ],
          'localPairingEnabled': 'boolean'
         },
         'results': {}
      }
    }
  })");

  const Settings& settings = dev_reg_->GetSettings();

  AddCommand(R"({
    'name' : 'base.updateBaseConfiguration',
    'parameters': {
      'localDiscoveryEnabled': false,
      'localAnonymousAccessMaxRole': 'none',
      'localPairingEnabled': false
    }
  })");
  EXPECT_EQ(AuthScope::kNone, settings.local_anonymous_access_role);
  EXPECT_FALSE(settings.local_discovery_enabled);
  EXPECT_FALSE(settings.local_pairing_enabled);

  auto expected = R"({
    'base': {
      'firmwareVersion': 'TEST_FIRMWARE',
      'localAnonymousAccessMaxRole': 'none',
      'localDiscoveryEnabled': false,
      'localPairingEnabled': false,
      'network': {}
    }
  })";
  EXPECT_JSON_EQ(expected, *state_manager_->GetState());

  AddCommand(R"({
    'name' : 'base.updateBaseConfiguration',
    'parameters': {
      'localDiscoveryEnabled': true,
      'localAnonymousAccessMaxRole': 'user',
      'localPairingEnabled': true
    }
  })");
  EXPECT_EQ(AuthScope::kUser, settings.local_anonymous_access_role);
  EXPECT_TRUE(settings.local_discovery_enabled);
  EXPECT_TRUE(settings.local_pairing_enabled);
  expected = R"({
    'base': {
      'firmwareVersion': 'TEST_FIRMWARE',
      'localAnonymousAccessMaxRole': 'user',
      'localDiscoveryEnabled': true,
      'localPairingEnabled': true,
      'network': {}
    }
  })";
  EXPECT_JSON_EQ(expected, *state_manager_->GetState());

  {
    Config::Transaction change{dev_reg_->GetMutableConfig()};
    change.set_local_anonymous_access_role(AuthScope::kViewer);
  }
  expected = R"({
    'base': {
      'firmwareVersion': 'TEST_FIRMWARE',
      'localAnonymousAccessMaxRole': 'viewer',
      'localDiscoveryEnabled': true,
      'localPairingEnabled': true,
      'network': {}
    }
  })";
  EXPECT_JSON_EQ(expected, *state_manager_->GetState());
}

TEST_F(BaseApiHandlerTest, UpdateDeviceInfo) {
  LoadCommands(R"({
    'base': {
      'updateDeviceInfo': {
        'parameters': {
          'description': 'string',
          'name': {
            'type': 'string',
            'minLength': 1
          },
          'location': 'string'
        },
        'results': {}
      }
    }
  })");

  AddCommand(R"({
    'name' : 'base.updateDeviceInfo',
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

  AddCommand(R"({
    'name' : 'base.updateDeviceInfo',
    'parameters': {
      'location': 'newLocation'
    }
  })");

  EXPECT_EQ("testName", config.name);
  EXPECT_EQ("testDescription", config.description);
  EXPECT_EQ("newLocation", config.location);
}

}  // namespace weave
