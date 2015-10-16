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

class GreeterHandler {
 public:
  GreeterHandler(provider::TaskRunner* task_runner)
      : task_runner_{task_runner} {}
  void Register(Device* device) {
    device_ = device;

    device->AddStateDefinitionsFromJson(R"({
      "_greeter": {"_greetings_counter":"integer"}
    })");

    device->SetStatePropertiesFromJson(R"({
      "_greeter": {"_greetings_counter": 0}
    })",
                                       nullptr);

    device->AddCommandDefinitionsFromJson(R"({
      "_greeter": {
        "_greet": {
          "minimalRole": "user",
          "parameters": {
            "_name": "string",
            "_count": {"minimum": 1, "maximum": 100}
          },
          "progress": { "_todo": "integer"},
          "results": { "_greeting": "string" }
        }
      }
    })");
    device->AddCommandHandler("_greeter._greet",
                              base::Bind(&GreeterHandler::OnGreetCommand,
                                         weak_ptr_factory_.GetWeakPtr()));
  }

 private:
  void DoGreet(const std::weak_ptr<Command>& command, int todo) {
    auto cmd = command.lock();
    if (!cmd)
      return;

    std::string name;
    if (!cmd->GetParameters()->GetString("_name", &name)) {
      ErrorPtr error;
      Error::AddTo(&error, FROM_HERE, "example",
                          "invalid_parameter_value", "Name is missing");
      cmd->Abort(error.get(), nullptr);
      return;
    }

    if (todo-- > 0) {
      LOG(INFO) << "Hello " << name;

      base::DictionaryValue progress;
      progress.SetInteger("_todo", todo);
      cmd->SetProgress(progress, nullptr);

      base::DictionaryValue state;
      state.SetInteger("_greeter._greetings_counter", ++counter_);
      device_->SetStateProperties(state, nullptr);
    }

    if (todo > 0) {
      task_runner_->PostDelayedTask(
          FROM_HERE, base::Bind(&GreeterHandler::DoGreet,
                                weak_ptr_factory_.GetWeakPtr(), command, todo),
          base::TimeDelta::FromSeconds(1));
      return;
    }

    base::DictionaryValue result;
    result.SetString("_greeting", "Hello " + name);
    cmd->Complete(result, nullptr);
    LOG(INFO) << cmd->GetName() << " command finished: " << result;
    LOG(INFO) << "New state: " << *device_->GetState();
  }

  void OnGreetCommand(const std::weak_ptr<Command>& command) {
    auto cmd = command.lock();
    if (!cmd)
      return;
    LOG(INFO) << "received command: " << cmd->GetName();

    int todo = 1;
    cmd->GetParameters()->GetInteger("_count", &todo);
    DoGreet(command, todo);
  }

  Device* device_{nullptr};
  provider::TaskRunner* task_runner_{nullptr};

  int counter_{0};
  base::WeakPtrFactory<GreeterHandler> weak_ptr_factory_{this};
};

}  // namespace daemon
}  // namespace examples
}  // namespace weave
