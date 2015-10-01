// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/privet/privet_manager.h"

#include <memory>
#include <set>
#include <string>

#include <base/bind.h>
#include <base/json/json_reader.h>
#include <base/json/json_writer.h>
#include <base/memory/weak_ptr.h>
#include <base/scoped_observer.h>
#include <base/strings/string_number_conversions.h>
#include <base/values.h>
#include <weave/provider/network.h>

#include "src/device_registration_info.h"
#include "src/http_constants.h"
#include "src/privet/cloud_delegate.h"
#include "src/privet/constants.h"
#include "src/privet/device_delegate.h"
#include "src/privet/privet_handler.h"
#include "src/privet/publisher.h"
#include "src/string_utils.h"

namespace weave {
namespace privet {

using provider::TaskRunner;
using provider::Network;
using provider::DnsServiceDiscovery;
using provider::HttpServer;
using provider::Wifi;

Manager::Manager() {}

Manager::~Manager() {}

void Manager::Start(TaskRunner* task_runner,
                    Network* network,
                    DnsServiceDiscovery* dns_sd,
                    HttpServer* http_server,
                    Wifi* wifi,
                    DeviceRegistrationInfo* device,
                    CommandManager* command_manager,
                    StateManager* state_manager) {
  disable_security_ = device->GetSettings().disable_security;

  device_ = DeviceDelegate::CreateDefault();
  cloud_ = CloudDelegate::CreateDefault(task_runner, device, command_manager,
                                        state_manager);
  cloud_observer_.Add(cloud_.get());
  security_.reset(new SecurityManager(
      device->GetSettings().secret, device->GetSettings().pairing_modes,
      device->GetSettings().embedded_code, disable_security_, task_runner));
  if (device->GetSettings().secret.empty()) {
    // TODO(vitalybuka): Post all Config::Transaction to avoid following.
    task_runner->PostDelayedTask(
        FROM_HERE,
        base::Bind(&Manager::SaveDeviceSecret, weak_ptr_factory_.GetWeakPtr(),
                   base::Unretained(device->GetMutableConfig())),
        {});
  }
  network->AddConnectionChangedCallback(
      base::Bind(&Manager::OnConnectivityChanged, base::Unretained(this)));

  if (wifi && device->GetSettings().wifi_auto_setup_enabled) {
    VLOG(1) << "Enabling WiFi bootstrapping.";
    wifi_bootstrap_manager_.reset(new WifiBootstrapManager(
        device->GetMutableConfig(), task_runner, network, wifi, cloud_.get()));
    wifi_bootstrap_manager_->Init();
  }

  if (dns_sd) {
    publisher_.reset(new Publisher(device_.get(), cloud_.get(),
                                   wifi_bootstrap_manager_.get(), dns_sd));
  }

  privet_handler_.reset(new PrivetHandler(cloud_.get(), device_.get(),
                                          security_.get(),
                                          wifi_bootstrap_manager_.get()));

  http_server->AddOnStateChangedCallback(base::Bind(
      &Manager::OnHttpServerStatusChanged, weak_ptr_factory_.GetWeakPtr()));
  http_server->AddRequestHandler("/privet/",
                                 base::Bind(&Manager::PrivetRequestHandler,
                                            weak_ptr_factory_.GetWeakPtr()));
}

std::string Manager::GetCurrentlyConnectedSsid() const {
  return wifi_bootstrap_manager_
             ? wifi_bootstrap_manager_->GetCurrentlyConnectedSsid()
             : "";
}

void Manager::AddOnPairingChangedCallbacks(
    const SecurityManager::PairingStartListener& on_start,
    const SecurityManager::PairingEndListener& on_end) {
  security_->RegisterPairingListeners(on_start, on_end);
}

void Manager::OnDeviceInfoChanged() {
  OnChanged();
}

void Manager::PrivetRequestHandler(
    const HttpServer::Request& request,
    const HttpServer::OnReplyCallback& callback) {
  std::string auth_header = request.GetFirstHeader(http::kAuthorization);
  if (auth_header.empty() && disable_security_)
    auth_header = "Privet anonymous";
  std::string data(request.GetData().begin(), request.GetData().end());
  VLOG(3) << "Input: " << data;

  base::DictionaryValue empty;
  std::unique_ptr<base::Value> value;
  const base::DictionaryValue* dictionary = &empty;

  std::string content_type =
      SplitAtFirst(request.GetFirstHeader(http::kContentType), ";", true).first;
  if (content_type == http::kJson) {
    value.reset(base::JSONReader::Read(data).release());
    if (value)
      value->GetAsDictionary(&dictionary);
  }

  privet_handler_->HandleRequest(
      request.GetPath(), auth_header, dictionary,
      base::Bind(&Manager::PrivetResponseHandler,
                 weak_ptr_factory_.GetWeakPtr(), callback));
}

void Manager::PrivetResponseHandler(const HttpServer::OnReplyCallback& callback,
                                    int status,
                                    const base::DictionaryValue& output) {
  VLOG(3) << "status: " << status << ", Output: " << output;
  std::string data;
  base::JSONWriter::WriteWithOptions(
      output, base::JSONWriter::OPTIONS_PRETTY_PRINT, &data);
  callback.Run(status, data, http::kJson);
}

void Manager::OnChanged() {
  if (publisher_)
    publisher_->Update();
}

void Manager::OnConnectivityChanged() {
  OnChanged();
}

void Manager::OnHttpServerStatusChanged(const HttpServer& server) {
  if (device_->GetHttpEnpoint().first != server.GetHttpPort()) {
    device_->SetHttpPort(server.GetHttpPort());
    if (publisher_)  // Only HTTP port is published
      publisher_->Update();
  }
  device_->SetHttpsPort(server.GetHttpsPort());
  security_->SetCertificateFingerprint(server.GetHttpsCertificateFingerprint());
}

void Manager::SaveDeviceSecret(Config* config) {
  Config::Transaction transaction(config);
  transaction.set_secret(security_->GetSecret());
}

}  // namespace privet
}  // namespace weave
