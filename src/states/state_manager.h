// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_STATES_STATE_MANAGER_H_
#define LIBWEAVE_SRC_STATES_STATE_MANAGER_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <base/callback.h>
#include <base/macros.h>
#include <weave/error.h>

#include "src/states/state_change_queue_interface.h"
#include "src/states/state_package.h"

namespace base {
class DictionaryValue;
class Time;
}  // namespace base

namespace weave {

// StateManager is the class that aggregates the device state fragments
// provided by device daemons and makes the aggregate device state available
// to the GCD cloud server and local clients.
class StateManager final {
 public:
  explicit StateManager(StateChangeQueueInterface* state_change_queue);
  ~StateManager();

  void AddChangedCallback(const base::Closure& callback);
  bool LoadStateDefinition(const base::DictionaryValue& dict, ErrorPtr* error);
  bool LoadStateDefinitionFromJson(const std::string& json, ErrorPtr* error);
  bool SetProperties(const base::DictionaryValue& dict, ErrorPtr* error);
  bool SetPropertiesFromJson(const std::string& json, ErrorPtr* error);
  std::unique_ptr<base::Value> GetProperty(const std::string& name) const;
  bool SetProperty(const std::string& name,
                   const base::Value& value,
                   ErrorPtr* error);
  std::unique_ptr<base::DictionaryValue> GetState() const;

  // Returns the recorded state changes since last time this method has been
  // called.
  std::pair<StateChangeQueueInterface::UpdateID, std::vector<StateChange>>
  GetAndClearRecordedStateChanges();

  // Called to notify that the state patch with |id| has been successfully sent
  // to the server and processed.
  void NotifyStateUpdatedOnServer(StateChangeQueueInterface::UpdateID id);

  StateChangeQueueInterface* GetStateChangeQueue() const {
    return state_change_queue_;
  }

 private:
  friend class BaseApiHandlerTest;
  friend class StateManagerTest;

  // Updates a single property value. |full_property_name| must be the full
  // name of the property to update in format "package.property".
  bool SetPropertyValue(const std::string& full_property_name,
                        const base::Value& value,
                        const base::Time& timestamp,
                        ErrorPtr* error);

  // Finds a package by its name. Returns nullptr if not found.
  StatePackage* FindPackage(const std::string& package_name);
  const StatePackage* FindPackage(const std::string& package_name) const;
  // Finds a package by its name. If none exists, one will be created.
  StatePackage* FindOrCreatePackage(const std::string& package_name);

  StateChangeQueueInterface* state_change_queue_;  // Owned by Manager.
  std::map<std::string, std::unique_ptr<StatePackage>> packages_;

  std::vector<base::Closure> on_changed_;

  DISALLOW_COPY_AND_ASSIGN(StateManager);
};

}  // namespace weave

#endif  // LIBWEAVE_SRC_STATES_STATE_MANAGER_H_
