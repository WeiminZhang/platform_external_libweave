// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_TEST_MOCK_DEVICE_H_
#define LIBWEAVE_INCLUDE_WEAVE_TEST_MOCK_DEVICE_H_

#include <weave/device.h>

#include <string>

#include <gmock/gmock.h>

namespace weave {
namespace test {

class MockDevice : public Device {
 public:
  ~MockDevice() override = default;

  MOCK_CONST_METHOD0(GetSettings, const Settings&());
  MOCK_METHOD1(AddSettingsChangedCallback,
               void(const SettingsChangedCallback& callback));
  MOCK_METHOD1(AddCommandDefinitionsFromJson, void(const std::string&));
  MOCK_METHOD1(AddCommandDefinitions, void(const base::DictionaryValue&));
  MOCK_METHOD2(AddCommandHandler,
               void(const std::string&, const CommandHandlerCallback&));
  MOCK_METHOD3(AddCommand,
               bool(const base::DictionaryValue&, std::string*, ErrorPtr*));
  MOCK_METHOD1(FindCommand, Command*(const std::string&));
  MOCK_METHOD1(AddStateChangedCallback, void(const base::Closure& callback));
  MOCK_METHOD1(AddStateDefinitionsFromJson, void(const std::string&));
  MOCK_METHOD1(AddStateDefinitions, void(const base::DictionaryValue&));
  MOCK_METHOD2(SetStatePropertiesFromJson, bool(const std::string&, ErrorPtr*));
  MOCK_METHOD2(SetStateProperties,
               bool(const base::DictionaryValue&, ErrorPtr*));
  MOCK_CONST_METHOD1(GetStateProperty,
                     const base::Value*(const std::string& name));
  MOCK_METHOD3(SetStateProperty,
               bool(const std::string& name,
                    const base::Value& value,
                    ErrorPtr* error));
  MOCK_CONST_METHOD0(GetState, const base::DictionaryValue&());
  MOCK_CONST_METHOD0(GetGcdState, GcdState());
  MOCK_METHOD1(AddGcdStateChangedCallback,
               void(const GcdStateChangedCallback& callback));
  MOCK_METHOD2(Register,
               void(const std::string& ticket_id,
                    const DoneCallback& callback));
  MOCK_METHOD2(AddPairingChangedCallbacks,
               void(const PairingBeginCallback& begin_callback,
                    const PairingEndCallback& end_callback));
};

}  // namespace test
}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_TEST_MOCK_DEVICE_H_
