// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <weave/test/mock_command.h>

#include <memory>
#include <string>

#include <base/values.h>
#include <weave/test/unittest_utils.h>

namespace weave {
namespace test {

std::unique_ptr<base::DictionaryValue> MockCommand::GetParameters() const {
  return CreateDictionaryValue(MockGetParameters());
}

std::unique_ptr<base::DictionaryValue> MockCommand::GetProgress() const {
  return CreateDictionaryValue(MockGetProgress());
}

std::unique_ptr<base::DictionaryValue> MockCommand::GetResults() const {
  return CreateDictionaryValue(MockGetResults());
}

}  // namespace test
}  // namespace weave
