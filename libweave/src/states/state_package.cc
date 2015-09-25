// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/states/state_package.h"

#include <base/logging.h>
#include <base/values.h>

#include "src/commands/prop_types.h"
#include "src/commands/prop_values.h"
#include "src/commands/schema_utils.h"
#include "src/states/error_codes.h"

namespace weave {

StatePackage::StatePackage(const std::string& name) : name_(name) {}

bool StatePackage::AddSchemaFromJson(const base::DictionaryValue* json,
                                     ErrorPtr* error) {
  ObjectSchema schema;
  if (!schema.FromJson(json, nullptr, error))
    return false;

  // Scan first to make sure we have no property redefinitions.
  for (const auto& pair : schema.GetProps()) {
    if (types_.GetProp(pair.first)) {
      Error::AddToPrintf(error, FROM_HERE, errors::state::kDomain,
                         errors::state::kPropertyRedefinition,
                         "State property '%s.%s' is already defined",
                         name_.c_str(), pair.first.c_str());
      return false;
    }
  }

  // Now move all the properties to |types_| object.
  for (const auto& pair : schema.GetProps()) {
    types_.AddProp(pair.first, pair.second->Clone());
    // Create default value for this state property.
    values_.emplace(pair.first, pair.second->CreateDefaultValue());
  }

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

std::unique_ptr<base::DictionaryValue> StatePackage::GetValuesAsJson() const {
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue);
  for (const auto& pair : values_) {
    auto value = pair.second->ToJson();
    CHECK(value);
    dict->SetWithoutPathExpansion(pair.first, value.release());
  }
  return dict;
}

std::unique_ptr<base::Value> StatePackage::GetPropertyValue(
    const std::string& property_name,
    ErrorPtr* error) const {
  auto it = values_.find(property_name);
  if (it == values_.end()) {
    Error::AddToPrintf(error, FROM_HERE, errors::state::kDomain,
                       errors::state::kPropertyNotDefined,
                       "State property '%s.%s' is not defined", name_.c_str(),
                       property_name.c_str());
    return nullptr;
  }

  return it->second->ToJson();
}

bool StatePackage::SetPropertyValue(const std::string& property_name,
                                    const base::Value& value,
                                    ErrorPtr* error) {
  auto it = values_.find(property_name);
  if (it == values_.end()) {
    Error::AddToPrintf(error, FROM_HERE, errors::state::kDomain,
                       errors::state::kPropertyNotDefined,
                       "State property '%s.%s' is not defined", name_.c_str(),
                       property_name.c_str());
    return false;
  }
  auto new_value = it->second->GetPropType()->CreatePropValue(value, error);
  if (!new_value)
    return false;
  it->second = std::move(new_value);
  return true;
}

}  // namespace weave
