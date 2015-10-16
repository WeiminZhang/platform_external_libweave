// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <weave/device.h>
#include <weave/error.h>

#include <base/bind.h>

#include "examples/daemon/ledflasher_handler.h"
#include "examples/daemon/sample_handler.h"

#include "examples/provider/avahi_client.h"
#include "examples/provider/bluez_client.h"
#include "examples/provider/curl_http_client.h"
#include "examples/provider/event_http_server.h"
#include "examples/provider/event_network.h"
#include "examples/provider/event_task_runner.h"
#include "examples/provider/file_config_store.h"
#include "examples/provider/wifi_manager.h"

namespace {

// Supported LED count on this device
const size_t kLedCount = 3;

void ShowUsage(const std::string& name) {
  LOG(ERROR) << "\nUsage: " << name << " <option(s)>"
             << "\nOptions:\n"
             << "\t-h,--help                    Show this help message\n"
             << "\t--v=LEVEL                    Logging level\n"
             << "\t-b,--bootstrapping           Force WiFi bootstrapping\n"
             << "\t-d,--disable_security        Disable privet security\n"
             << "\t--registration_ticket=TICKET Register device with the "
                "given ticket\n"
             << "\t--disable_privet             Disable local privet\n";
}

void OnRegisterDeviceDone(weave::Device* device, weave::ErrorPtr error) {
  if (error)
    LOG(ERROR) << "Fail to register device: " << error->GetMessage();
  else
    LOG(INFO) << "Device registered: " << device->GetSettings().cloud_id;
}

}  // namespace

int main(int argc, char** argv) {
  bool force_bootstrapping = false;
  bool disable_security = false;
  bool disable_privet = false;
  std::string registration_ticket;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-h" || arg == "--help") {
      ShowUsage(argv[0]);
      return 0;
    } else if (arg == "-b" || arg == "--bootstrapping") {
      force_bootstrapping = true;
    } else if (arg == "-d" || arg == "--disable_security") {
      disable_security = true;
    } else if (arg == "--disable_privet") {
      disable_privet = true;
    } else if (arg.find("--registration_ticket") != std::string::npos) {
      auto pos = arg.find("=");
      if (pos == std::string::npos) {
        ShowUsage(argv[0]);
        return 1;
      }
      registration_ticket = arg.substr(pos + 1);
    } else if (arg.find("--v") != std::string::npos) {
      auto pos = arg.find("=");
      if (pos == std::string::npos) {
        ShowUsage(argv[0]);
        return 1;
      }
      logging::SetMinLogLevel(-std::stoi(arg.substr(pos + 1)));
    } else {
      ShowUsage(argv[0]);
      return 1;
    }
  }

  weave::examples::FileConfigStore config_store{disable_security};
  weave::examples::EventTaskRunner task_runner;
  weave::examples::CurlHttpClient http_client{&task_runner};
  weave::examples::EventNetworkImpl network{&task_runner};
  weave::examples::BluetoothImpl bluetooth;
  std::unique_ptr<weave::examples::AvahiClient> dns_sd;
  std::unique_ptr<weave::examples::HttpServerImpl> http_server;
  std::unique_ptr<weave::examples::WifiImpl> wifi;

  if (!disable_privet) {
    dns_sd.reset(new weave::examples::AvahiClient);
    http_server.reset(new weave::examples::HttpServerImpl{&task_runner});
    if (weave::examples::WifiImpl::HasWifiCapability())
      wifi.reset(
          new weave::examples::WifiImpl{&task_runner, force_bootstrapping});
  }
  std::unique_ptr<weave::Device> device{weave::Device::Create(
      &config_store, &task_runner, &http_client, &network, dns_sd.get(),
      http_server.get(), wifi.get(), &bluetooth)};

  if (!registration_ticket.empty()) {
    device->Register(registration_ticket,
                     base::Bind(&OnRegisterDeviceDone, device.get()));
  }

  weave::examples::daemon::SampleHandler sample{&task_runner};
  weave::examples::daemon::LedFlasherHandler ledFlasher;
  sample.Register(device.get());
  ledFlasher.Register(device.get());

  task_runner.Run();

  LOG(INFO) << "exit";
  return 0;
}
