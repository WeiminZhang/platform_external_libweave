// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/states/state_manager.h"

#include <base/logging.h>
#include <base/values.h>

#include "src/json_error_codes.h"
#include "src/states/error_codes.h"
#include "src/states/state_change_queue_interface.h"
#include "src/string_utils.h"
#include "src/utils.h"

namespace weave {

StateManager::StateManager(StateChangeQueueInterface* state_change_queue)
    : state_change_queue_(state_change_queue) {
  CHECK(state_change_queue_) << "State change queue not specified";
}

StateManager::~StateManager() {}

void StateManager::AddChangedCallback(const base::Closure& callback) {
  on_changed_.push_back(callback);
  callback.Run();  // Force to read current state.
}

std::unique_ptr<base::DictionaryValue> StateManager::GetState() const {
  std::unique_ptr<base::DictionaryValue> dict{new base::DictionaryValue};
  for (const auto& pair : packages_) {
    auto pkg_value = pair.second->GetValuesAsJson();
    CHECK(pkg_value);
    dict->SetWithoutPathExpansion(pair.first, pkg_value.release());
  }
  return dict;
}

bool StateManager::SetProperty(const std::string& name,
                               const base::Value& value,
                               ErrorPtr* error) {
  bool result = SetPropertyValue(name, value, base::Time::Now(), error);
  for (const auto& cb : on_changed_)
    cb.Run();
  return result;
}

std::unique_ptr<base::Value> StateManager::GetProperty(
    const std::string& name) const {
  auto parts = SplitAtFirst(name, ".", true);
  const std::string& package_name = parts.first;
  const std::string& property_name = parts.second;
  if (package_name.empty() || property_name.empty())
    return nullptr;

  const StatePackage* package = FindPackage(package_name);
  if (!package)
    return nullptr;

  return package->GetPropertyValue(property_name, nullptr);
}

bool StateManager::SetPropertyValue(const std::string& full_property_name,
                                    const base::Value& value,
                                    const base::Time& timestamp,
                                    ErrorPtr* error) {
  auto parts = SplitAtFirst(full_property_name, ".", true);
  const std::string& package_name = parts.first;
  const std::string& property_name = parts.second;
  const bool split = (full_property_name.find(".") != std::string::npos);

  if (full_property_name.empty() || (split && property_name.empty())) {
    Error::AddTo(error, FROM_HERE, errors::state::kDomain,
                 errors::state::kPropertyNameMissing,
                 "Property name is missing");
    return false;
  }
  if (!split || package_name.empty()) {
    Error::AddTo(error, FROM_HERE, errors::state::kDomain,
                 errors::state::kPackageNameMissing,
                 "Package name is missing in the property name");
    return false;
  }
  StatePackage* package = FindPackage(package_name);
  if (package == nullptr) {
    Error::AddToPrintf(error, FROM_HERE, errors::state::kDomain,
                       errors::state::kPropertyNotDefined,
                       "Unknown state property package '%s'",
                       package_name.c_str());
    return false;
  }
  if (!package->SetPropertyValue(property_name, value, error))
    return false;

  std::unique_ptr<base::DictionaryValue> prop_set{new base::DictionaryValue};
  prop_set->Set(full_property_name, value.DeepCopy());
  state_change_queue_->NotifyPropertiesUpdated(timestamp, std::move(prop_set));
  return true;
}

std::pair<StateChangeQueueInterface::UpdateID, std::vector<StateChange>>
StateManager::GetAndClearRecordedStateChanges() {
  return std::make_pair(state_change_queue_->GetLastStateChangeId(),
                        state_change_queue_->GetAndClearRecordedStateChanges());
}

void StateManager::NotifyStateUpdatedOnServer(
    StateChangeQueueInterface::UpdateID id) {
  state_change_queue_->NotifyStateUpdatedOnServer(id);
}

bool StateManager::LoadStateDefinition(const base::DictionaryValue& dict,
                                       ErrorPtr* error) {
  for (base::DictionaryValue::Iterator iter(dict); !iter.IsAtEnd();
       iter.Advance()) {
    std::string package_name = iter.key();
    if (package_name.empty()) {
      Error::AddTo(error, FROM_HERE, errors::kErrorDomain,
                   errors::kInvalidPackageError, "State package name is empty");
      return false;
    }
    const base::DictionaryValue* package_dict = nullptr;
    if (!iter.value().GetAsDictionary(&package_dict)) {
      Error::AddToPrintf(error, FROM_HERE, errors::json::kDomain,
                         errors::json::kObjectExpected,
                         "State package '%s' must be an object",
                         package_name.c_str());
      return false;
    }
    StatePackage* package = FindOrCreatePackage(package_name);
    CHECK(package) << "Unable to create state package " << package_name;
    if (!package->AddSchemaFromJson(package_dict, error))
      return false;
  }

  return true;
}

bool StateManager::LoadStateDefinitionFromJson(const std::string& json,
                                               ErrorPtr* error) {
  std::unique_ptr<const base::DictionaryValue> dict = LoadJsonDict(json, error);
  if (!dict)
    return false;
  if (!LoadStateDefinition(*dict, error)) {
    Error::AddToPrintf(error, FROM_HERE, errors::kErrorDomain,
                       errors::kSchemaError,
                       "Failed to load state definition: '%s'", json.c_str());
    return false;
  }
  return true;
}

bool StateManager::SetProperties(const base::DictionaryValue& dict,
                                 ErrorPtr* error) {
  base::Time timestamp = base::Time::Now();
  bool all_success = true;
  for (base::DictionaryValue::Iterator iter(dict); !iter.IsAtEnd();
       iter.Advance()) {
    if (iter.key().empty()) {
      Error::AddTo(error, FROM_HERE, errors::kErrorDomain,
                   errors::kInvalidPackageError, "State package name is empty");
      all_success = false;
      continue;
    }

    const base::DictionaryValue* package_dict = nullptr;
    if (!iter.value().GetAsDictionary(&package_dict)) {
      Error::AddToPrintf(error, FROM_HERE, errors::json::kDomain,
                         errors::json::kObjectExpected,
                         "State package '%s' must be an object",
                         iter.key().c_str());
      all_success = false;
      continue;
    }

    for (base::DictionaryValue::Iterator it_prop(*package_dict);
         !it_prop.IsAtEnd(); it_prop.Advance()) {
      if (!SetPropertyValue(iter.key() + "." + it_prop.key(), it_prop.value(),
                            timestamp, error)) {
        all_success = false;
        continue;
      }
    }
  }
  for (const auto& cb : on_changed_)
    cb.Run();
  return all_success;
}

bool StateManager::SetPropertiesFromJson(const std::string& json,
                                         ErrorPtr* error) {
  std::unique_ptr<const base::DictionaryValue> dict = LoadJsonDict(json, error);
  if (!dict)
    return false;
  if (!SetProperties(*dict, error)) {
    Error::AddToPrintf(error, FROM_HERE, errors::kErrorDomain,
                       errors::kSchemaError, "Failed to load defaults: '%s'",
                       json.c_str());
    return false;
  }
  return true;
}

StatePackage* StateManager::FindPackage(const std::string& package_name) {
  auto it = packages_.find(package_name);
  return (it != packages_.end()) ? it->second.get() : nullptr;
}

const StatePackage* StateManager::FindPackage(
    const std::string& package_name) const {
  auto it = packages_.find(package_name);
  return (it != packages_.end()) ? it->second.get() : nullptr;
}

StatePackage* StateManager::FindOrCreatePackage(
    const std::string& package_name) {
  StatePackage* package = FindPackage(package_name);
  if (package == nullptr) {
    std::unique_ptr<StatePackage> new_package{new StatePackage(package_name)};
    package =
        packages_.insert(std::make_pair(package_name, std::move(new_package)))
            .first->second.get();
  }
  return package;
}

}  // namespace weave
