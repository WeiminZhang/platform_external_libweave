// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <base/bind.h>
#include <base/values.h>
#include <weave/device.h>
#include <weave/error.h>

#include "examples/ubuntu/avahi_client.h"
#include "examples/ubuntu/bluez_client.h"
#include "examples/ubuntu/curl_http_client.h"
#include "examples/ubuntu/event_http_server.h"
#include "examples/ubuntu/event_task_runner.h"
#include "examples/ubuntu/file_config_store.h"
#include "examples/ubuntu/network_manager.h"

namespace {
void ShowUsage(const std::string& name) {
  LOG(ERROR) << "\nUsage: " << name << " <option(s)>"
             << "\nOptions:\n"
             << "\t-h,--help                    Show this help message\n"
             << "\t-b,--bootstrapping           Force WiFi bootstrapping\n"
             << "\t--disable_security           Disable privet security\n"
             << "\t--registration_ticket=TICKET Register device with the given "
                "ticket\n";
}

class CommandHandler {
 public:
  explicit CommandHandler(weave::Device* device) : device_{device} {
    device->AddCommandAddedCallback(base::Bind(&CommandHandler::OnNewCommand,
                                               weak_ptr_factory_.GetWeakPtr()));
  }

 private:
  void OnNewCommand(weave::Command* cmd) {
    LOG(INFO) << "received command: " << cmd->GetName();
    if (cmd->GetName() == "_greeter._greet") {
      std::string name;
      if (!cmd->GetParameters()->GetString("_name", &name))
        name = "anonymous";

      LOG(INFO) << cmd->GetName() << " command in progress";
      cmd->SetProgress(base::DictionaryValue{}, nullptr);

      base::DictionaryValue result;
      result.SetString("_greeting", "Hello " + name);
      cmd->SetResults(result, nullptr);
      LOG(INFO) << cmd->GetName() << " command finished: " << result;

      base::DictionaryValue state;
      state.SetIntegerWithoutPathExpansion("_greeter._greetings_counter",
                                           ++counter_);
      device_->SetStateProperties(state, nullptr);

      LOG(INFO) << "New state: " << *device_->GetState();

      cmd->Done();
    } else {
      LOG(INFO) << "unimplemented command: ignored";
    }
  }

  weave::Device* device_{nullptr};
  int counter_{0};

  base::WeakPtrFactory<CommandHandler> weak_ptr_factory_{this};
};
}

int main(int argc, char** argv) {
  bool force_bootstrapping = false;
  bool disable_security = false;
  std::string registration_ticket;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-h" || arg == "--help") {
      ShowUsage(argv[0]);
      return 0;
    } else if (arg == "-b" || arg == "--bootstrapping") {
      force_bootstrapping = true;
    } else if (arg == "--disable_security") {
      disable_security = true;
    } else if (arg.find("--registration_ticket") != std::string::npos) {
      auto pos = arg.find("=");
      if (pos == std::string::npos) {
        ShowUsage(argv[0]);
        return 1;
      }
      registration_ticket = arg.substr(pos + 1);
    } else {
      ShowUsage(argv[0]);
      return 1;
    }
  }

  weave::examples::FileConfigStore config_store{disable_security};
  weave::examples::EventTaskRunner task_runner;
  weave::examples::CurlHttpClient http_client{&task_runner};
  weave::examples::NetworkImpl network{&task_runner, force_bootstrapping};
  weave::examples::AvahiClient dns_sd;
  weave::examples::HttpServerImpl http_server{&task_runner};
  weave::examples::BluetoothImpl bluetooth;

  auto device = weave::Device::Create(
      &config_store, &task_runner, &http_client, &network, &dns_sd,
      &http_server,
      weave::examples::NetworkImpl::HasWifiCapability() ? &network : nullptr,
      &bluetooth);

  if (!registration_ticket.empty()) {
    weave::ErrorPtr error;
    auto device_id = device->Register(registration_ticket, &error);
    if (error != nullptr) {
      LOG(ERROR) << "Fail to register device: " << error->GetMessage();
    } else {
      LOG(INFO) << "Device registered: " << device_id;
    }
  }

  CommandHandler handler(device.get());
  task_runner.Run();

  LOG(INFO) << "exit";
  return 0;
}
