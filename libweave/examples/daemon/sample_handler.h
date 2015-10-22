// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <weave/device.h>
#include <weave/provider/task_runner.h>

#include <base/bind.h>
#include <base/memory/weak_ptr.h>

namespace weave {
namespace examples {
namespace daemon {

// SampleHandler is a command handler example.
// It implements the following commands:
// - _hello: handle a command with an argument and set its results.
// - _ping: update device state.
// - _countdown: handle long running command and report progress.
class SampleHandler {
 public:
  SampleHandler(provider::TaskRunner* task_runner)
      : task_runner_{task_runner} {}
  void Register(Device* device) {
    device_ = device;

    device->AddCommandDefinitionsFromJson(R"({
      "_sample": {
        "_hello": {
          "minimalRole": "user",
          "parameters": {
            "_name": "string"
          },
          "results": { "_reply": "string" }
        },
        "_ping": {
          "minimalRole": "user",
          "results": {}
        },
        "_countdown": {
          "minimalRole": "user",
          "parameters": {
            "_seconds": {"minimum": 1, "maximum": 25}
          },
          "progress": { "_seconds_left": "integer"},
          "results": {}
        }
      }
    })");

    device->AddStateDefinitionsFromJson(R"({
      "_sample": {"_ping_count":"integer"}
    })");

    device->SetStatePropertiesFromJson(R"({
      "_sample": {"_ping_count": 0}
    })",
                                       nullptr);

    device->AddCommandHandler("_sample._hello",
                              base::Bind(&SampleHandler::OnHelloCommand,
                                         weak_ptr_factory_.GetWeakPtr()));
    device->AddCommandHandler("_sample._ping",
                              base::Bind(&SampleHandler::OnPingCommand,
                                         weak_ptr_factory_.GetWeakPtr()));
    device->AddCommandHandler("_sample._countdown",
                              base::Bind(&SampleHandler::OnCountdownCommand,
                                         weak_ptr_factory_.GetWeakPtr()));
  }

 private:
  void OnHelloCommand(const std::weak_ptr<Command>& command) {
    auto cmd = command.lock();
    if (!cmd)
      return;
    LOG(INFO) << "received command: " << cmd->GetName();

    std::string name;
    if (!cmd->GetParameters()->GetString("_name", &name)) {
      ErrorPtr error;
      Error::AddTo(&error, FROM_HERE, "example", "invalid_parameter_value",
                   "Name is missing");
      cmd->Abort(error.get(), nullptr);
      return;
    }

    base::DictionaryValue result;
    result.SetString("_reply", "Hello " + name);
    cmd->Complete(result, nullptr);
    LOG(INFO) << cmd->GetName() << " command finished: " << result;
  }

  void OnPingCommand(const std::weak_ptr<Command>& command) {
    auto cmd = command.lock();
    if (!cmd)
      return;
    LOG(INFO) << "received command: " << cmd->GetName();

    base::DictionaryValue state;
    state.SetInteger("_sample._ping_count", ++ping_count_);
    device_->SetStateProperties(state, nullptr);
    LOG(INFO) << "New state: " << *device_->GetState();

    base::DictionaryValue result;
    cmd->Complete(result, nullptr);

    LOG(INFO) << cmd->GetName() << " command finished: " << result;
  }

  void OnCountdownCommand(const std::weak_ptr<Command>& command) {
    auto cmd = command.lock();
    if (!cmd)
      return;
    LOG(INFO) << "received command: " << cmd->GetName();

    int seconds;
    if (!cmd->GetParameters()->GetInteger("_seconds", &seconds))
      seconds = 10;

    LOG(INFO) << "starting countdown";
    DoTick(cmd, seconds);
  }

  void DoTick(const std::weak_ptr<Command>& command, int seconds) {
    auto cmd = command.lock();
    if (!cmd)
      return;

    if (seconds > 0) {
      std::string todo;
      cmd->GetParameters()->GetString("_todo", &todo);
      LOG(INFO) << "countdown tick: " << seconds << " seconds left";

      base::DictionaryValue progress;
      progress.SetInteger("_seconds_left", seconds);
      cmd->SetProgress(progress, nullptr);
      task_runner_->PostDelayedTask(
          FROM_HERE,
          base::Bind(&SampleHandler::DoTick, weak_ptr_factory_.GetWeakPtr(),
                     command, --seconds),
          base::TimeDelta::FromSeconds(1));
      return;
    }

    base::DictionaryValue result;
    cmd->Complete(result, nullptr);
    LOG(INFO) << "countdown finished";
    LOG(INFO) << cmd->GetName() << " command finished: " << result;
  }

  Device* device_{nullptr};
  provider::TaskRunner* task_runner_{nullptr};

  int ping_count_{0};
  base::WeakPtrFactory<SampleHandler> weak_ptr_factory_{this};
};

}  // namespace daemon
}  // namespace examples
}  // namespace weave
