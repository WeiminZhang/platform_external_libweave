// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/component_manager.h"

#include <base/strings/stringprintf.h>
#include <base/strings/string_number_conversions.h>
#include <base/strings/string_util.h>

#include "src/commands/schema_constants.h"
#include "src/string_utils.h"
#include "src/utils.h"

namespace weave {

ComponentManager::ComponentManager() {}
ComponentManager::~ComponentManager() {}

bool ComponentManager::AddComponent(const std::string& path,
                                    const std::string& name,
                                    const std::vector<std::string>& traits,
                                    ErrorPtr* error) {
  base::DictionaryValue* root = &components_;
  if (!path.empty()) {
    root = FindComponentGraftNode(path, error);
    if (!root)
      return false;
  }
  if (root->GetWithoutPathExpansion(name, nullptr)) {
    Error::AddToPrintf(error, FROM_HERE, errors::commands::kDomain,
                       errors::commands::kInvalidState,
                       "Component '%s' already exists at path '%s'",
                       name.c_str(), path.c_str());
    return false;
  }

  // Check to make sure the declared traits are already defined.
  for (const std::string& trait : traits) {
    if (!FindTraitDefinition(trait)) {
      Error::AddToPrintf(error, FROM_HERE, errors::commands::kDomain,
                         errors::commands::kInvalidPropValue,
                         "Trait '%s' is undefined", trait.c_str());
      return false;
    }
  }
  std::unique_ptr<base::DictionaryValue> dict{new base::DictionaryValue};
  std::unique_ptr<base::ListValue> traits_list{new base::ListValue};
  traits_list->AppendStrings(traits);
  dict->Set("traits", traits_list.release());
  root->SetWithoutPathExpansion(name, dict.release());
  return true;
}

bool ComponentManager::AddComponentArrayItem(
    const std::string& path,
    const std::string& name,
    const std::vector<std::string>& traits,
    ErrorPtr* error) {
  base::DictionaryValue* root = &components_;
  if (!path.empty()) {
    root = FindComponentGraftNode(path, error);
    if (!root)
      return false;
  }
  base::ListValue* array_value = nullptr;
  if (!root->GetListWithoutPathExpansion(name, &array_value)) {
    array_value = new base::ListValue;
    root->SetWithoutPathExpansion(name, array_value);
  }
  std::unique_ptr<base::DictionaryValue> dict{new base::DictionaryValue};
  std::unique_ptr<base::ListValue> traits_list{new base::ListValue};
  traits_list->AppendStrings(traits);
  dict->Set("traits", traits_list.release());
  array_value->Append(dict.release());
  return true;
}

bool ComponentManager::LoadTraits(const base::DictionaryValue& dict,
                                  ErrorPtr* error) {
  bool modified = false;
  bool result = true;
  // Check if any of the new traits are already defined. If so, make sure the
  // definition is exactly the same, or else this is an error.
  for (base::DictionaryValue::Iterator it(dict); !it.IsAtEnd(); it.Advance()) {
    if (it.value().GetType() != base::Value::TYPE_DICTIONARY) {
      Error::AddToPrintf(error, FROM_HERE, errors::commands::kDomain,
                         errors::commands::kTypeMismatch,
                         "Trait '%s' must be an object", it.key().c_str());
      result = false;
      break;
    }
    const base::DictionaryValue* existing_def = nullptr;
    if (traits_.GetDictionary(it.key(), &existing_def)) {
      if (!existing_def->Equals(&it.value())) {
        Error::AddToPrintf(error, FROM_HERE, errors::commands::kDomain,
                           errors::commands::kTypeMismatch,
                           "Trait '%s' cannot be redefined",
                           it.key().c_str());
        result = false;
        break;
      }
    }
    traits_.Set(it.key(), it.value().DeepCopy());
    modified = true;
  }

  if (modified) {
    for (const auto& cb : on_trait_changed_)
      cb.Run();
  }
  return result;
}

bool ComponentManager::LoadTraits(const std::string& json, ErrorPtr* error) {
  std::unique_ptr<const base::DictionaryValue> dict = LoadJsonDict(json, error);
  if (!dict)
    return false;
  return LoadTraits(*dict, error);
}

void ComponentManager::AddTraitDefChanged(const base::Closure& callback) {
  on_trait_changed_.push_back(callback);
  callback.Run();
}

void ComponentManager::AddCommand(
    std::unique_ptr<CommandInstance> command_instance) {
  command_queue_.Add(std::move(command_instance));
}

bool ComponentManager::AddCommand(const base::DictionaryValue& command,
                                  UserRole role,
                                  std::string* id,
                                  ErrorPtr* error) {
  auto command_instance = CommandInstance::FromJson(
      &command, Command::Origin::kLocal, nullptr, error);
  if (!command_instance)
    return false;

  UserRole minimal_role;
  if (!GetMinimalRole(command_instance->GetName(), &minimal_role, error))
    return false;

  if (role < minimal_role) {
    Error::AddToPrintf(
        error, FROM_HERE, errors::commands::kDomain, "access_denied",
        "User role '%s' less than minimal: '%s'", EnumToString(role).c_str(),
        EnumToString(minimal_role).c_str());
    return false;
  }

  std::string component_path = command_instance->GetComponent();
  if (component_path.empty()) {
    // Get the name of the first top-level component.
    base::DictionaryValue::Iterator it(components_);
    if (it.IsAtEnd()) {
      Error::AddTo(error, FROM_HERE, errors::commands::kDomain,
                   "component_not_found", "There are no components defined");
      return false;
    }
    component_path = it.key();
    command_instance->SetComponent(component_path);
  }

  const auto* component = FindComponent(component_path, error);
  if (!component)
    return false;

  // Check that the command's trait is supported by the given component.
  auto pair = SplitAtFirst(command_instance->GetName(), ".", true);

  bool trait_supported = false;
  const base::ListValue* supported_traits = nullptr;
  if (component->GetList("traits", &supported_traits)) {
    for (const base::Value* value : *supported_traits) {
      std::string trait;
      CHECK(value->GetAsString(&trait));
      if (trait == pair.first) {
        trait_supported = true;
        break;
      }
    }
  }

  if (!trait_supported) {
    Error::AddToPrintf(error, FROM_HERE, errors::commands::kDomain,
                       "trait_not_supported",
                       "Component '%s' doesn't support trait '%s'",
                       component_path.c_str(), pair.first.c_str());
    return false;
  }

  std::string command_id = std::to_string(++next_command_id_);
  command_instance->SetID(command_id);
  if (id)
    *id = command_id;
  AddCommand(std::move(command_instance));
  return true;
}

CommandInstance* ComponentManager::FindCommand(const std::string& id) {
  return command_queue_.Find(id);
}

void ComponentManager::AddCommandAddedCallback(
    const CommandQueue::CommandCallback& callback) {
  command_queue_.AddCommandAddedCallback(callback);
}

void ComponentManager::AddCommandRemovedCallback(
    const CommandQueue::CommandCallback& callback) {
  command_queue_.AddCommandRemovedCallback(callback);
}

void ComponentManager::AddCommandHandler(
    const std::string& component_path,
    const std::string& command_name,
    const Device::CommandHandlerCallback& callback) {
  CHECK(FindCommandDefinition(command_name))
      << "Command undefined: " << command_name;
  command_queue_.AddCommandHandler(component_path, command_name, callback);
}

const base::DictionaryValue* ComponentManager::FindComponent(
    const std::string& path, ErrorPtr* error) const {
  return FindComponentAt(&components_, path, error);
}

const base::DictionaryValue* ComponentManager::FindTraitDefinition(
    const std::string& name) const {
  const base::DictionaryValue* trait = nullptr;
  traits_.GetDictionaryWithoutPathExpansion(name, &trait);
  return trait;
}

const base::DictionaryValue* ComponentManager::FindCommandDefinition(
    const std::string& command_name) const {
  const base::DictionaryValue* definition = nullptr;
  std::vector<std::string> components = Split(command_name, ".", true, false);
  // Make sure the |command_name| came in form of trait_name.command_name.
  if (components.size() != 2)
    return definition;
  std::string key = base::StringPrintf("%s.commands.%s", components[0].c_str(),
                                       components[1].c_str());
  traits_.GetDictionary(key, &definition);
  return definition;
}

bool ComponentManager::GetMinimalRole(const std::string& command_name,
                                      UserRole* minimal_role,
                                      ErrorPtr* error) const {
  const base::DictionaryValue* command = FindCommandDefinition(command_name);
  if (!command) {
    Error::AddToPrintf(error, FROM_HERE, errors::commands::kDomain,
                       errors::commands::kInvalidCommandName,
                       "Command definition for '%s' not found",
                       command_name.c_str());
    return false;
  }
  std::string value;
  // The JSON definition has been pre-validated already in LoadCommands, so
  // just using CHECKs here.
  CHECK(command->GetString(commands::attributes::kCommand_Role, &value));
  CHECK(StringToEnum(value, minimal_role));
  return true;
}

base::DictionaryValue* ComponentManager::FindComponentGraftNode(
    const std::string& path, ErrorPtr* error) {
  base::DictionaryValue* root = nullptr;
  auto component = const_cast<base::DictionaryValue*>(FindComponentAt(
      &components_, path, error));
  if (component && !component->GetDictionary("components", &root)) {
    root = new base::DictionaryValue;
    component->Set("components", root);
  }
  return root;
}

const base::DictionaryValue* ComponentManager::FindComponentAt(
    const base::DictionaryValue* root,
    const std::string& path,
    ErrorPtr* error) {
  auto parts = Split(path, ".", true, false);
  std::string root_path;
  for (size_t i = 0; i < parts.size(); i++) {
    auto element = SplitAtFirst(parts[i], "[", true);
    int array_index = -1;
    if (element.first.empty()) {
      Error::AddToPrintf(error, FROM_HERE, errors::commands::kDomain,
                         errors::commands::kPropertyMissing,
                         "Empty path element at '%s'", root_path.c_str());
      return nullptr;
    }
    if (!element.second.empty()) {
      if (element.second.back() != ']') {
        Error::AddToPrintf(error, FROM_HERE, errors::commands::kDomain,
                           errors::commands::kPropertyMissing,
                           "Invalid array element syntax '%s'",
                           parts[i].c_str());
        return nullptr;
      }
      element.second.pop_back();
      std::string index_str;
      base::TrimWhitespaceASCII(element.second, base::TrimPositions::TRIM_ALL,
                                &index_str);
      if (!base::StringToInt(index_str, &array_index) || array_index < 0) {
        Error::AddToPrintf(error, FROM_HERE, errors::commands::kDomain,
                           errors::commands::kInvalidPropValue,
                           "Invalid array index '%s'", element.second.c_str());
        return nullptr;
      }
    }

    if (!root_path.empty()) {
      // We have processed at least one item in the path before, so now |root|
      // points to the actual parent component. We need the root to point to
      // the 'components' element containing child sub-components instead.
      if (!root->GetDictionary("components", &root)) {
        Error::AddToPrintf(error, FROM_HERE, errors::commands::kDomain,
                           errors::commands::kPropertyMissing,
                           "Component '%s' does not exist at '%s'",
                           element.first.c_str(), root_path.c_str());
        return nullptr;
      }
    }

    const base::Value* value = nullptr;
    if (!root->GetWithoutPathExpansion(element.first, &value)) {
      Error::AddToPrintf(error, FROM_HERE, errors::commands::kDomain,
                         errors::commands::kPropertyMissing,
                         "Component '%s' does not exist at '%s'",
                         element.first.c_str(), root_path.c_str());
      return nullptr;
    }

    if (value->GetType() == base::Value::TYPE_LIST && array_index < 0) {
      Error::AddToPrintf(error, FROM_HERE, errors::commands::kDomain,
                         errors::commands::kTypeMismatch,
                         "Element '%s.%s' is an array",
                         root_path.c_str(), element.first.c_str());
      return nullptr;
    }
    if (value->GetType() == base::Value::TYPE_DICTIONARY && array_index >= 0) {
      Error::AddToPrintf(error, FROM_HERE, errors::commands::kDomain,
                         errors::commands::kTypeMismatch,
                         "Element '%s.%s' is not an array",
                         root_path.c_str(), element.first.c_str());
      return nullptr;
    }

    if (value->GetType() == base::Value::TYPE_DICTIONARY) {
      CHECK(value->GetAsDictionary(&root));
    } else {
      const base::ListValue* component_array = nullptr;
      CHECK(value->GetAsList(&component_array));
      const base::Value* component_value = nullptr;
      if (!component_array->Get(array_index, &component_value) ||
          !component_value->GetAsDictionary(&root)) {
        Error::AddToPrintf(error, FROM_HERE, errors::commands::kDomain,
                           errors::commands::kPropertyMissing,
                           "Element '%s.%s' does not contain item #%d",
                           root_path.c_str(), element.first.c_str(),
                           array_index);
        return nullptr;
      }
    }
    if (!root_path.empty())
      root_path += '.';
    root_path += parts[i];
  }
  return root;
}

}  // namespace weave
