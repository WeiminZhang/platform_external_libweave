// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/states/state_package.h"

#include <base/logging.h>
#include <base/values.h>

#include "src/states/error_codes.h"

namespace weave {

StatePackage::StatePackage(const std::string& name) : name_(name) {}

bool StatePackage::AddSchemaFromJson(const base::DictionaryValue* json,
                                     ErrorPtr* error) {
  // Scan first to make sure we have no property redefinitions.
  for (base::DictionaryValue::Iterator it(*json); !it.IsAtEnd(); it.Advance()) {
    if (types_.HasKey(it.key())) {
      Error::AddToPrintf(error, FROM_HERE, errors::state::kDomain,
                         errors::state::kPropertyRedefinition,
                         "State property '%s.%s' is already defined",
                         name_.c_str(), it.key().c_str());
      return false;
    }
  }

  types_.MergeDictionary(json);
  return true;
}

bool StatePackage::AddValuesFromJson(const base::DictionaryValue* json,
                                     ErrorPtr* error) {
  for (base::DictionaryValue::Iterator it(*json); !it.IsAtEnd(); it.Advance()) {
    if (!SetPropertyValue(it.key(), it.value(), error))
      return false;
  }
  return true;
}

const base::DictionaryValue& StatePackage::GetValuesAsJson() const {
  return values_;
}

const base::Value* StatePackage::GetPropertyValue(
    const std::string& property_name,
    ErrorPtr* error) const {
  const base::Value* value = nullptr;
  if (!values_.Get(property_name, &value)) {
    Error::AddToPrintf(error, FROM_HERE, errors::state::kDomain,
                       errors::state::kPropertyNotDefined,
                       "State property '%s.%s' is not defined", name_.c_str(),
                       property_name.c_str());
    return nullptr;
  }

  return value;
}

bool StatePackage::SetPropertyValue(const std::string& property_name,
                                    const base::Value& value,
                                    ErrorPtr* error) {
  values_.Set(property_name, value.DeepCopy());
  return true;
}

}  // namespace weave
