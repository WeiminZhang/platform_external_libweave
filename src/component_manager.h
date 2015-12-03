// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_COMPONENT_MANAGER_H_
#define LIBWEAVE_SRC_COMPONENT_MANAGER_H_

#include <memory>

#include <base/values.h>
#include <weave/error.h>

#include "src/commands/command_dictionary.h"
#include "src/commands/command_queue.h"

namespace weave {

class CommandInstance;

class ComponentManager final {
 public:
  ComponentManager();
  ~ComponentManager();

  // Loads trait definition schema.
  bool LoadTraits(const base::DictionaryValue& dict, ErrorPtr* error);

  // Same as the overload above, but takes a json string to read the trait
  // definitions from.
  bool LoadTraits(const std::string& json, ErrorPtr* error);

  // Sets callback which is called when new trait definitions are added.
  void AddTraitDefChanged(const base::Closure& callback);

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

 private:
  // A helper method to find a JSON element of component at |path| to add new
  // sub-components to.
  base::DictionaryValue* FindComponentGraftNode(const std::string& path,
                                                ErrorPtr* error);

  // Helper method to find a sub-component given a root node and a relative path
  // from the root to the target component.
  static const base::DictionaryValue* FindComponentAt(
      const base::DictionaryValue* root,
      const std::string& path,
      ErrorPtr* error);


  base::DictionaryValue traits_;  // Trait definitions.
  base::DictionaryValue components_;  // Component instances.
  CommandQueue command_queue_;  // Command queue containing command instances.
  std::vector<base::Callback<void()>> on_trait_changed_;
  uint32_t next_command_id_{0};

  DISALLOW_COPY_AND_ASSIGN(ComponentManager);
};

}  // namespace weave

#endif  // LIBWEAVE_SRC_COMPONENT_MANAGER_H_
