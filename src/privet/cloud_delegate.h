// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_PRIVET_CLOUD_DELEGATE_H_
#define LIBWEAVE_SRC_PRIVET_CLOUD_DELEGATE_H_

#include <memory>
#include <set>
#include <string>

#include <base/callback.h>
#include <base/memory/ref_counted.h>
#include <base/observer_list.h>

#include "src/privet/privet_types.h"
#include "src/privet/security_delegate.h"

namespace base {
class DictionaryValue;
}  // namespace base

namespace weave {

class CommandManager;
class DeviceRegistrationInfo;
class StateManager;

namespace provider {
class TaskRunner;
}

namespace privet {

// Interface to provide GCD functionality for PrivetHandler.
// TODO(vitalybuka): Rename to BuffetDelegate.
class CloudDelegate {
 public:
  CloudDelegate();
  virtual ~CloudDelegate();

  using CommandDoneCallback =
      base::Callback<void(const base::DictionaryValue& commands,
                          ErrorPtr error)>;

  class Observer {
   public:
    virtual ~Observer() = default;

    virtual void OnDeviceInfoChanged() {}
    virtual void OnCommandDefsChanged() {}
    virtual void OnStateChanged() {}
  };

  // Returns the ID of the device.
  virtual std::string GetDeviceId() const = 0;

  // Returns the model ID of the device.
  virtual std::string GetModelId() const = 0;

  // Returns the name of device.
  virtual std::string GetName() const = 0;

  // Returns the description of the device.
  virtual std::string GetDescription() const = 0;

  // Returns the location of the device.
  virtual std::string GetLocation() const = 0;

  // Update basic device information.
  virtual void UpdateDeviceInfo(const std::string& name,
                                const std::string& description,
                                const std::string& location) = 0;

  // Returns the name of the maker.
  virtual std::string GetOemName() const = 0;

  // Returns the model name of the device.
  virtual std::string GetModelName() const = 0;

  // Returns max scope available for anonymous user.
  virtual AuthScope GetAnonymousMaxScope() const = 0;

  // Returns status of the GCD connection.
  virtual const ConnectionState& GetConnectionState() const = 0;

  // Returns status of the last setup.
  virtual const SetupState& GetSetupState() const = 0;

  // Starts GCD setup.
  virtual bool Setup(const std::string& ticket_id,
                     const std::string& user,
                     ErrorPtr* error) = 0;

  // Returns cloud id if the registered device or empty string if unregistered.
  virtual std::string GetCloudId() const = 0;

  // Returns dictionary with device state.
  virtual const base::DictionaryValue& GetState() const = 0;

  // Returns dictionary with commands definitions.
  virtual const base::DictionaryValue& GetCommandDef() const = 0;

  // Adds command created from the given JSON representation.
  virtual void AddCommand(const base::DictionaryValue& command,
                          const UserInfo& user_info,
                          const CommandDoneCallback& callback) = 0;

  // Returns command with the given ID.
  virtual void GetCommand(const std::string& id,
                          const UserInfo& user_info,
                          const CommandDoneCallback& callback) = 0;

  // Cancels command with the given ID.
  virtual void CancelCommand(const std::string& id,
                             const UserInfo& user_info,
                             const CommandDoneCallback& callback) = 0;

  // Lists commands.
  virtual void ListCommands(const UserInfo& user_info,
                            const CommandDoneCallback& callback) = 0;

  void AddObserver(Observer* observer) { observer_list_.AddObserver(observer); }
  void RemoveObserver(Observer* observer) {
    observer_list_.RemoveObserver(observer);
  }

  void NotifyOnDeviceInfoChanged();
  void NotifyOnCommandDefsChanged();
  void NotifyOnStateChanged();

  // Create default instance.
  static std::unique_ptr<CloudDelegate> CreateDefault(
      provider::TaskRunner* task_runner,
      DeviceRegistrationInfo* device,
      CommandManager* command_manager,
      StateManager* state_manager);

 private:
  base::ObserverList<Observer> observer_list_;
};

}  // namespace privet
}  // namespace weave

#endif  // LIBWEAVE_SRC_PRIVET_CLOUD_DELEGATE_H_
