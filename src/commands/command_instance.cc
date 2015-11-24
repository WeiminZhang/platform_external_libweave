// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/commands/command_instance.h"

#include <base/values.h>
#include <weave/enum_to_string.h>
#include <weave/error.h>
#include <weave/export.h>

#include "src/commands/command_definition.h"
#include "src/commands/command_dictionary.h"
#include "src/commands/command_queue.h"
#include "src/commands/prop_types.h"
#include "src/commands/schema_constants.h"
#include "src/commands/schema_utils.h"
#include "src/json_error_codes.h"
#include "src/utils.h"

namespace weave {

namespace {

const EnumToStringMap<Command::State>::Map kMapStatus[] = {
    {Command::State::kQueued, "queued"},
    {Command::State::kInProgress, "inProgress"},
    {Command::State::kPaused, "paused"},
    {Command::State::kError, "error"},
    {Command::State::kDone, "done"},
    {Command::State::kCancelled, "cancelled"},
    {Command::State::kAborted, "aborted"},
    {Command::State::kExpired, "expired"},
};

const EnumToStringMap<Command::Origin>::Map kMapOrigin[] = {
    {Command::Origin::kLocal, "local"},
    {Command::Origin::kCloud, "cloud"},
};

bool ReportDestroyedError(ErrorPtr* error) {
  Error::AddTo(error, FROM_HERE, errors::commands::kDomain,
               errors::commands::kCommandDestroyed,
               "Command has been destroyed");
  return false;
}

bool ReportInvalidStateTransition(ErrorPtr* error,
                                  Command::State from,
                                  Command::State to) {
  Error::AddToPrintf(error, FROM_HERE, errors::commands::kDomain,
                     errors::commands::kInvalidState,
                     "State switch impossible: '%s' -> '%s'",
                     EnumToString(from).c_str(), EnumToString(to).c_str());
  return false;
}

}  // namespace

template <>
LIBWEAVE_EXPORT EnumToStringMap<Command::State>::EnumToStringMap()
    : EnumToStringMap(kMapStatus) {}

template <>
LIBWEAVE_EXPORT EnumToStringMap<Command::Origin>::EnumToStringMap()
    : EnumToStringMap(kMapOrigin) {}

CommandInstance::CommandInstance(const std::string& name,
                                 Command::Origin origin,
                                 const CommandDefinition* command_definition,
                                 const ValueMap& parameters)
    : name_{name},
      origin_{origin},
      command_definition_{command_definition},
      parameters_{parameters} {
  CHECK(command_definition_);
}

CommandInstance::~CommandInstance() {
  FOR_EACH_OBSERVER(Observer, observers_, OnCommandDestroyed());
}

const std::string& CommandInstance::GetID() const {
  return id_;
}

const std::string& CommandInstance::GetName() const {
  return name_;
}

Command::State CommandInstance::GetState() const {
  return state_;
}

Command::Origin CommandInstance::GetOrigin() const {
  return origin_;
}

std::unique_ptr<base::DictionaryValue> CommandInstance::GetParameters() const {
  return TypedValueToJson(parameters_);
}

std::unique_ptr<base::DictionaryValue> CommandInstance::GetProgress() const {
  return TypedValueToJson(progress_);
}

std::unique_ptr<base::DictionaryValue> CommandInstance::GetResults() const {
  return TypedValueToJson(results_);
}

const Error* CommandInstance::GetError() const {
  return error_.get();
}

bool CommandInstance::SetProgress(const base::DictionaryValue& progress,
                                  ErrorPtr* error) {
  if (!command_definition_)
    return ReportDestroyedError(error);
  ObjectPropType obj_prop_type;
  obj_prop_type.SetObjectSchema(command_definition_->GetProgress()->Clone());

  ValueMap obj;
  if (!TypedValueFromJson(&progress, &obj_prop_type, &obj, error))
    return false;

  // Change status even if progress unchanged, e.g. 0% -> 0%.
  if (!SetStatus(State::kInProgress, error))
    return false;

  if (obj != progress_) {
    progress_ = obj;
    FOR_EACH_OBSERVER(Observer, observers_, OnProgressChanged());
  }

  return true;
}

bool CommandInstance::Complete(const base::DictionaryValue& results,
                               ErrorPtr* error) {
  if (!command_definition_)
    return ReportDestroyedError(error);
  ObjectPropType obj_prop_type;
  obj_prop_type.SetObjectSchema(command_definition_->GetResults()->Clone());

  ValueMap obj;
  if (!TypedValueFromJson(&results, &obj_prop_type, &obj, error))
    return false;

  if (obj != results_) {
    results_ = obj;
    FOR_EACH_OBSERVER(Observer, observers_, OnResultsChanged());
  }
  // Change status even if result is unchanged.
  bool result = SetStatus(State::kDone, error);
  RemoveFromQueue();
  // The command will be destroyed after that, so do not access any members.
  return result;
}

bool CommandInstance::SetError(const Error* command_error, ErrorPtr* error) {
  error_ = command_error ? command_error->Clone() : nullptr;
  FOR_EACH_OBSERVER(Observer, observers_, OnErrorChanged());
  return SetStatus(State::kError, error);
}

namespace {

// Helper method to retrieve command parameters from the command definition
// object passed in as |json| and corresponding command definition schema
// specified in |command_def|.
// On success, returns |true| and the validated parameters and values through
// |parameters|. Otherwise returns |false| and additional error information in
// |error|.
bool GetCommandParameters(const base::DictionaryValue* json,
                          const CommandDefinition* command_def,
                          ValueMap* parameters,
                          ErrorPtr* error) {
  // Get the command parameters from 'parameters' property.
  base::DictionaryValue no_params;  // Placeholder when no params are specified.
  const base::DictionaryValue* params = nullptr;
  const base::Value* params_value = nullptr;
  if (json->Get(commands::attributes::kCommand_Parameters, &params_value)) {
    // Make sure the "parameters" property is actually an object.
    if (!params_value->GetAsDictionary(&params)) {
      Error::AddToPrintf(error, FROM_HERE, errors::json::kDomain,
                         errors::json::kObjectExpected,
                         "Property '%s' must be a JSON object",
                         commands::attributes::kCommand_Parameters);
      return false;
    }
  } else {
    // "parameters" are not specified. Assume empty param list.
    params = &no_params;
  }

  // Now read in the parameters and validate their values against the command
  // definition schema.
  ObjectPropType obj_prop_type;
  obj_prop_type.SetObjectSchema(command_def->GetParameters()->Clone());
  if (!TypedValueFromJson(params, &obj_prop_type, parameters, error)) {
    return false;
  }
  return true;
}

}  // anonymous namespace

std::unique_ptr<CommandInstance> CommandInstance::FromJson(
    const base::Value* value,
    Command::Origin origin,
    const CommandDictionary& dictionary,
    std::string* command_id,
    ErrorPtr* error) {
  std::unique_ptr<CommandInstance> instance;
  std::string command_id_buffer;  // used if |command_id| was nullptr.
  if (!command_id)
    command_id = &command_id_buffer;

  // Get the command JSON object from the value.
  const base::DictionaryValue* json = nullptr;
  if (!value->GetAsDictionary(&json)) {
    Error::AddTo(error, FROM_HERE, errors::json::kDomain,
                 errors::json::kObjectExpected,
                 "Command instance is not a JSON object");
    command_id->clear();
    return instance;
  }

  // Get the command ID from 'id' property.
  if (!json->GetString(commands::attributes::kCommand_Id, command_id))
    command_id->clear();

  // Get the command name from 'name' property.
  std::string command_name;
  if (!json->GetString(commands::attributes::kCommand_Name, &command_name)) {
    Error::AddTo(error, FROM_HERE, errors::commands::kDomain,
                 errors::commands::kPropertyMissing, "Command name is missing");
    return instance;
  }
  // Make sure we know how to handle the command with this name.
  auto command_def = dictionary.FindCommand(command_name);
  if (!command_def) {
    Error::AddToPrintf(error, FROM_HERE, errors::commands::kDomain,
                       errors::commands::kInvalidCommandName,
                       "Unknown command received: %s", command_name.c_str());
    return instance;
  }

  ValueMap parameters;
  if (!GetCommandParameters(json, command_def, &parameters, error)) {
    Error::AddToPrintf(error, FROM_HERE, errors::commands::kDomain,
                       errors::commands::kCommandFailed,
                       "Failed to validate command '%s'", command_name.c_str());
    return instance;
  }

  instance.reset(
      new CommandInstance{command_name, origin, command_def, parameters});

  if (!command_id->empty())
    instance->SetID(*command_id);

  return instance;
}

std::unique_ptr<base::DictionaryValue> CommandInstance::ToJson() const {
  std::unique_ptr<base::DictionaryValue> json{new base::DictionaryValue};

  json->SetString(commands::attributes::kCommand_Id, id_);
  json->SetString(commands::attributes::kCommand_Name, name_);
  json->Set(commands::attributes::kCommand_Parameters,
            TypedValueToJson(parameters_).release());
  json->Set(commands::attributes::kCommand_Progress,
            TypedValueToJson(progress_).release());
  json->Set(commands::attributes::kCommand_Results,
            TypedValueToJson(results_).release());
  json->SetString(commands::attributes::kCommand_State, EnumToString(state_));
  if (error_) {
    json->Set(commands::attributes::kCommand_Error,
              ErrorInfoToJson(*error_).release());
  }

  return json;
}

void CommandInstance::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void CommandInstance::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

bool CommandInstance::Pause(ErrorPtr* error) {
  return SetStatus(State::kPaused, error);
}

bool CommandInstance::Abort(const Error* command_error, ErrorPtr* error) {
  error_ = command_error ? command_error->Clone() : nullptr;
  FOR_EACH_OBSERVER(Observer, observers_, OnErrorChanged());
  bool result = SetStatus(State::kAborted, error);
  RemoveFromQueue();
  // The command will be destroyed after that, so do not access any members.
  return result;
}

bool CommandInstance::Cancel(ErrorPtr* error) {
  bool result = SetStatus(State::kCancelled, error);
  RemoveFromQueue();
  // The command will be destroyed after that, so do not access any members.
  return result;
}

bool CommandInstance::SetStatus(Command::State status, ErrorPtr* error) {
  if (status == state_)
    return true;
  if (status == State::kQueued)
    return ReportInvalidStateTransition(error, state_, status);
  switch (state_) {
    case State::kDone:
    case State::kCancelled:
    case State::kAborted:
    case State::kExpired:
      return ReportInvalidStateTransition(error, state_, status);
    case State::kQueued:
    case State::kInProgress:
    case State::kPaused:
    case State::kError:
      break;
  }
  state_ = status;
  FOR_EACH_OBSERVER(Observer, observers_, OnStateChanged());
  return true;
}

void CommandInstance::RemoveFromQueue() {
  if (queue_)
    queue_->DelayedRemove(GetID());
}

}  // namespace weave
