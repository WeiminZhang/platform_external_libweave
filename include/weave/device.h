// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_DEVICE_H_
#define LIBWEAVE_INCLUDE_WEAVE_DEVICE_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include <weave/command.h>
#include <weave/export.h>
#include <weave/provider/bluetooth.h>
#include <weave/provider/config_store.h>
#include <weave/provider/dns_service_discovery.h>
#include <weave/provider/http_client.h>
#include <weave/provider/http_server.h>
#include <weave/provider/network.h>
#include <weave/provider/task_runner.h>
#include <weave/provider/wifi.h>

namespace weave {

// This interface defines interactions with a Weave device.  Implemented in
// weave::DeviceManager.  The general use of this class is to:
// - Define the traits for the Weave schema the device is
//    interested in using the AddTraitDefinitions() and
//    AddTraitDefinitionsFromJson() functions.
// - Define the components for the Weave schema the device is
//    interested in using the AddComponent() function.
// - Set the initial/default values of "state" portions of the schema using
//    the SetStatePropertiesFromJson() and SetStateProperties() functions.
// - Specify callback functions when a command (defined in the Weave schema)
//    is received by the device using the AddCommandHandler() function.
//
// A daemon will typically create one instance of a class implementing
// Device and will retain it for the life of the daemon using the Create()
// function.
//
// For detailed examples of the usage of this class, see the examples/daemon
// directory.

// States of Gcd connection.
enum class GcdState {
  kUnconfigured,        // Device was not registered.
  kConnecting,          // We have credentials but not yet connected.
  kConnected,           // We're registered and connected to the cloud.
  kInvalidCredentials,  // Our registration has been revoked.
};

struct RegistrationData {
  explicit RegistrationData(const std::string& ticket = {})
      : ticket_id{ticket} {}

  std::string ticket_id;

  std::string oauth_url;
  std::string client_id;
  std::string client_secret;
  std::string api_key;
  std::string service_url;
  std::string xmpp_endpoint;
};

inline bool operator==(const RegistrationData& l, const RegistrationData& r) {
  return l.ticket_id == r.ticket_id && l.oauth_url == r.oauth_url &&
         l.client_id == r.client_id && l.client_secret == r.client_secret &&
         l.api_key == r.api_key && l.service_url == r.service_url &&
         l.xmpp_endpoint == r.xmpp_endpoint;
}

inline bool operator!=(const RegistrationData& l, const RegistrationData& r) {
  return !(l == r);
}

class Device {
 public:
  virtual ~Device() {}

  // Returns reference the current settings.
  virtual const Settings& GetSettings() const = 0;

  // Callback type for AddSettingsChangedCallback.
  using SettingsChangedCallback =
      base::Callback<void(const Settings& settings)>;

  // Subscribes to notification settings changes.
  virtual void AddSettingsChangedCallback(
      const SettingsChangedCallback& callback) = 0;

  // Adds new trait definitions to device.
  virtual void AddTraitDefinitionsFromJson(const std::string& json) = 0;
  virtual void AddTraitDefinitions(const base::DictionaryValue& dict) = 0;

  // Returns the full JSON dictionary containing trait definitions.
  virtual const base::DictionaryValue& GetTraits() const = 0;

  // Sets callback which is called when new trait definitions are added.
  virtual void AddTraitDefsChangedCallback(const base::Closure& callback) = 0;

  // Adds a new component instance to device. Traits used by this component
  // must be already defined.
  virtual bool AddComponent(const std::string& name,
                            const std::vector<std::string>& traits,
                            ErrorPtr* error) = 0;

  // Removes an existing component instance from device.
  virtual bool RemoveComponent(const std::string& name,
                               ErrorPtr* error) = 0;

  // Sets callback which is called when new components are added.
  virtual void AddComponentTreeChangedCallback(
      const base::Closure& callback) = 0;

  // Returns the full JSON dictionary containing component instances.
  virtual const base::DictionaryValue& GetComponents() const = 0;

  // Sets value of multiple properties of the state.
  // It's recommended to call this to initialize component state defined.
  // Example:
  //   device->SetStatePropertiesFromJson("myComponent",
  //                                      "{'base':{'firmwareVersion':'123'}}")
  // Method completely replaces properties included |json| or |dict|.
  // Properties of the state not included |json| or |dict| will stay unchanged.
  virtual bool SetStatePropertiesFromJson(const std::string& component,
                                          const std::string& json,
                                          ErrorPtr* error) = 0;
  virtual bool SetStateProperties(const std::string& component,
                                  const base::DictionaryValue& dict,
                                  ErrorPtr* error) = 0;

  // Returns value of the single property.
  // |name| is full property name, including trait name. e.g. "base.network".
  virtual const base::Value* GetStateProperty(const std::string& component,
                                              const std::string& name,
                                              ErrorPtr* error) const = 0;

  // Sets value of the single property.
  // |name| is full property name, including trait name. e.g. "base.network".
  virtual bool SetStateProperty(const std::string& component,
                                const std::string& name,
                                const base::Value& value,
                                ErrorPtr* error) = 0;

  // Callback type for AddCommandHandler.
  using CommandHandlerCallback =
      base::Callback<void(const std::weak_ptr<Command>& command)>;

  // Sets handler for new commands added to the queue.
  // |component| is the name of the component for which commands should be
  // handled.
  // |command_name| is the full command name of the command to handle. e.g.
  // "base.reboot". Each command can have no more than one handler.
  // Empty |component| and |command_name| sets default handler for all unhanded
  // commands.
  // No new command handlers can be set after default handler was set.
  virtual void AddCommandHandler(const std::string& component,
                                 const std::string& command_name,
                                 const CommandHandlerCallback& callback) = 0;

  // Adds a new command to the command queue.
  virtual bool AddCommand(const base::DictionaryValue& command,
                          std::string* id,
                          ErrorPtr* error) = 0;

  // Finds a command by the command |id|. Returns nullptr if the command with
  // the given |id| is not found. The returned pointer should not be persisted
  // for a long period of time.
  virtual Command* FindCommand(const std::string& id) = 0;

  // Sets callback which is called when stat is changed.
  virtual void AddStateChangedCallback(const base::Closure& callback) = 0;

  // Returns current state of GCD connection.
  virtual GcdState GetGcdState() const = 0;

  // Callback type for GcdStatusCallback.
  using GcdStateChangedCallback = base::Callback<void(GcdState state)>;

  // Sets callback which is called when state of server connection changed.
  virtual void AddGcdStateChangedCallback(
      const GcdStateChangedCallback& callback) = 0;

  // Registers the device.
  // This is testing method and should not be used by applications.
  virtual void Register(const RegistrationData& registration_data,
                        const DoneCallback& callback) = 0;

  // Handler should display pin code to the user.
  using PairingBeginCallback =
      base::Callback<void(const std::string& session_id,
                          PairingType pairing_type,
                          const std::vector<uint8_t>& code)>;

  // Handler should stop displaying pin code.
  using PairingEndCallback =
      base::Callback<void(const std::string& session_id)>;
  // Subscribes to notification about client pairing events.

  virtual void AddPairingChangedCallbacks(
      const PairingBeginCallback& begin_callback,
      const PairingEndCallback& end_callback) = 0;

  LIBWEAVE_EXPORT static std::unique_ptr<Device> Create(
      provider::ConfigStore* config_store,
      provider::TaskRunner* task_runner,
      provider::HttpClient* http_client,
      provider::Network* network,
      provider::DnsServiceDiscovery* dns_sd,
      provider::HttpServer* http_server,
      provider::Wifi* wifi,
      provider::Bluetooth* bluetooth_provider);
};

}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_DEVICE_H_
