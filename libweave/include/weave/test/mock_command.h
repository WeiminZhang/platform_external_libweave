// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_TEST_MOCK_COMMAND_H_
#define LIBWEAVE_INCLUDE_WEAVE_TEST_MOCK_COMMAND_H_

#include <weave/command.h>

#include <memory>
#include <string>

#include <base/values.h>
#include <gmock/gmock.h>

namespace weave {
namespace test {

class MockCommand : public Command {
 public:
  ~MockCommand() override = default;

  MOCK_CONST_METHOD0(GetID, const std::string&());
  MOCK_CONST_METHOD0(GetName, const std::string&());
  MOCK_CONST_METHOD0(GetCategory, const std::string&());
  MOCK_CONST_METHOD0(GetState, Command::State());
  MOCK_CONST_METHOD0(GetOrigin, Command::Origin());
  MOCK_CONST_METHOD0(MockGetParameters, const std::string&());
  MOCK_CONST_METHOD0(MockGetProgress, const std::string&());
  MOCK_CONST_METHOD0(MockGetResults, const std::string&());
  MOCK_CONST_METHOD0(GetError, const Error*());
  MOCK_METHOD2(SetProgress, bool(const base::DictionaryValue&, ErrorPtr*));
  MOCK_METHOD2(SetResults, bool(const base::DictionaryValue&, ErrorPtr*));
  MOCK_METHOD1(Pause, bool(ErrorPtr*));
  MOCK_METHOD2(SetError, bool(const Error*, ErrorPtr*));
  MOCK_METHOD2(Abort, bool(const Error*, ErrorPtr*));
  MOCK_METHOD1(Cancel, bool(ErrorPtr*));

  std::unique_ptr<base::DictionaryValue> GetParameters() const override;
  std::unique_ptr<base::DictionaryValue> GetProgress() const override;
  std::unique_ptr<base::DictionaryValue> GetResults() const override;
};

}  // namespace test
}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_TEST_MOCK_COMMAND_H_
