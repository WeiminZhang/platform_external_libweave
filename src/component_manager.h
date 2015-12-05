// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_COMPONENT_MANAGER_H_
#define LIBWEAVE_SRC_COMPONENT_MANAGER_H_

#include <map>
#include <memory>

#include <base/callback_list.h>
#include <base/time/clock.h>
#include <base/values.h>
#include <weave/error.h>

#include "src/commands/command_dictionary.h"
#include "src/commands/command_queue.h"
#include "src/states/state_change_queue.h"

namespace weave {

class CommandInstance;

// A simple notification record event to track component state changes.
// The |timestamp| records the time of the state change.
// |changed_properties| contains a property set with the new property values
// which were updated at the time the event was recorded.
struct ComponentStateChange {
  ComponentStateChange(base::Time time,
                       const std::string& path,
                       std::unique_ptr<base::DictionaryValue> properties)
      : timestamp{time}, component{path},
        changed_properties{std::move(properties)} {}
  base::Time timestamp;
  std::string component;
  std::unique_ptr<base::DictionaryValue> changed_properties;
};

class ComponentManager final {
 public:
  using UpdateID = uint64_t;
  using Token =
      std::unique_ptr<base::CallbackList<void(UpdateID)>::Subscription>;
  struct StateSnapshot {
    UpdateID update_id;
    std::vector<ComponentStateChange> state_changes;
  };

  ComponentManager();
  explicit ComponentManager(base::Clock* clock);
  ~ComponentManager();

  // Loads trait definition schema.
  bool LoadTraits(const base::DictionaryValue& dict, ErrorPtr* error);

  // Same as the overload above, but takes a json string to read the trait
  // definitions from.
  bool LoadTraits(const std::string& json, ErrorPtr* error);

  // Sets callback which is called when new trait definitions are added.
  void AddTraitDefChangedCallback(const base::Closure& callback);

  // Adds a new component instance to device.
  // |path| is a path to the parent component (or empty string if a root-level
  // component is being added).
  // |name| is a component name being added.
  // |traits| is a list of trait names this component supports.
  bool AddComponent(const std::string& path,
                    const std::string& name,
                    const std::vector<std::string>& traits,
                    ErrorPtr* error);

  // Adds a new component instance to device, as a part of component array.
  // |path| is a path to the parent component.
  // |name| is an array root element inside the child components.
  // |traits| is a list of trait names this component supports.
  bool AddComponentArrayItem(const std::string& path,
                             const std::string& name,
                             const std::vector<std::string>& traits,
                             ErrorPtr* error);

  // Sets callback which is called when new components are added.
  void AddComponentTreeChangedCallback(const base::Closure& callback);

  // Adds a new command instance to the command queue. The command specified in
  // |command_instance| must be fully initialized and have its name, component,
  // id populated.
  void AddCommand(std::unique_ptr<CommandInstance> command_instance);

  // Parses the command definition from a json dictionary and adds it to the
  // command queue. The new command ID is returned through optional |id| param.
  bool AddCommand(const base::DictionaryValue& command,
                  UserRole role,
                  std::string* id,
                  ErrorPtr* error);

  // Find a command instance with the given ID in the command queue.
  CommandInstance* FindCommand(const std::string& id);

  // Command queue monitoring callbacks (called when a new command is added to
  // or removed from the queue).
  void AddCommandAddedCallback(const CommandQueue::CommandCallback& callback);
  void AddCommandRemovedCallback(const CommandQueue::CommandCallback& callback);

  // Adds a command handler for specific component's command.
  // |component_path| is a path to target component (e.g. "stove.burners[2]").
  // |command_name| is a full path of the command, including trait name and
  // command name (e.g. "burner.setPower").
  void AddCommandHandler(const std::string& component_path,
                         const std::string& command_name,
                         const Device::CommandHandlerCallback& callback);

  // Finds a component instance by its full path.
  const base::DictionaryValue* FindComponent(const std::string& path,
                                             ErrorPtr* error) const;
  // Finds a definition of trait with the given |name|.
  const base::DictionaryValue* FindTraitDefinition(
      const std::string& name) const;

  // Finds a command definition, where |command_name| is in the form of
  // "trait.command".
  const base::DictionaryValue* FindCommandDefinition(
      const std::string& command_name) const;

  // Checks the minimum required user role for a given command.
  bool GetMinimalRole(const std::string& command_name,
                      UserRole* minimal_role,
                      ErrorPtr* error) const;

  // Returns the full JSON dictionary containing trait definitions.
  const base::DictionaryValue& GetTraits() const { return traits_; }

  // Returns the full JSON dictionary containing component instances.
  const base::DictionaryValue& GetComponents() const { return components_; }

  // Component state manipulation methods.
  bool SetStateProperties(const std::string& component_path,
                          const base::DictionaryValue& dict,
                          ErrorPtr* error);
  bool SetStatePropertiesFromJson(const std::string& component_path,
                                  const std::string& json,
                                  ErrorPtr* error);
  const base::Value* GetStateProperty(const std::string& component_path,
                                      const std::string& name,
                                      ErrorPtr* error) const;
  bool SetStateProperty(const std::string& component_path,
                        const std::string& name,
                        const base::Value& value,
                        ErrorPtr* error);

  void AddStateChangedCallback(const base::Closure& callback);

  // Returns the recorded state changes since last time this method was called.
  StateSnapshot GetAndClearRecordedStateChanges();

  // Called to notify that the state patch with |id| has been successfully sent
  // to the server and processed.
  void NotifyStateUpdatedOnServer(UpdateID id);

  // Returns an ID of last state change update. Each SetStatePropertyNNN()
  // invocation increments this value by 1.
  UpdateID GetLastStateChangeId() const { return last_state_change_id_; }

  // Subscribes for device state update notifications from cloud server.
  // The |callback| will be called every time a state patch with given ID is
  // successfully received and processed by Weave server.
  // Returns a subscription token. As soon as this token is destroyed, the
  // respective callback is removed from the callback list.
  Token AddServerStateUpdatedCallback(
      const base::Callback<void(UpdateID)>& callback);

  // Helper method for legacy API to obtain first component that implements
  // the given trait. This is useful for routing commands that have no component
  // path specified.
  // Returns empty string if no components are found.
  // This method only searches for component on the top level of components
  // tree. No sub-components are searched.
  std::string FindComponentWithTrait(const std::string& trait) const;

 private:
  // A helper method to find a JSON element of component at |path| to add new
  // sub-components to.
  base::DictionaryValue* FindComponentGraftNode(const std::string& path,
                                                ErrorPtr* error);
  base::DictionaryValue* FindMutableComponent(const std::string& path,
                                              ErrorPtr* error);

  // Helper method to find a sub-component given a root node and a relative path
  // from the root to the target component.
  static const base::DictionaryValue* FindComponentAt(
      const base::DictionaryValue* root,
      const std::string& path,
      ErrorPtr* error);

  base::Clock* clock_{nullptr};
  base::DictionaryValue traits_;  // Trait definitions.
  base::DictionaryValue components_;  // Component instances.
  CommandQueue command_queue_;  // Command queue containing command instances.
  std::vector<base::Closure> on_trait_changed_;
  std::vector<base::Closure> on_componet_tree_changed_;
  std::vector<base::Closure> on_state_changed_;
  uint32_t next_command_id_{0};

  std::map<std::string, std::unique_ptr<StateChangeQueue>> state_change_queues_;
  // An ID of last state change update. Each NotifyPropertiesUpdated()
  // invocation increments this value by 1.
  UpdateID last_state_change_id_{0};
  // Callback list for state change queue event sinks.
  base::CallbackList<void(UpdateID)> on_server_state_updated_;

  DISALLOW_COPY_AND_ASSIGN(ComponentManager);
};

}  // namespace weave

#endif  // LIBWEAVE_SRC_COMPONENT_MANAGER_H_
