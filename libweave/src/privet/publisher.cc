// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "libweave/src/privet/publisher.h"

#include <map>

#include <weave/error.h>
#include <weave/mdns.h>

#include "libweave/src/privet/cloud_delegate.h"
#include "libweave/src/privet/device_delegate.h"
#include "libweave/src/privet/wifi_bootstrap_manager.h"
#include "libweave/src/privet/wifi_ssid_generator.h"
#include "libweave/src/string_utils.h"

namespace weave {
namespace privet {

namespace {

// The service type we'll expose via mdns.
const char kPrivetServiceType[] = "_privet._tcp";

}  // namespace

Publisher::Publisher(const DeviceDelegate* device,
                     const CloudDelegate* cloud,
                     const WifiDelegate* wifi,
                     Mdns* mdns)
    : mdns_{mdns}, device_{device}, cloud_{cloud}, wifi_{wifi} {
  CHECK(device_);
  CHECK(cloud_);
  CHECK(mdns_);
}

Publisher::~Publisher() {
  RemoveService();
}

std::string Publisher::GetId() const {
  return mdns_->GetId();
}

void Publisher::Update() {
  if (device_->GetHttpEnpoint().first == 0)
    return RemoveService();
  ExposeService();
}

void Publisher::ExposeService() {
  std::string name;
  std::string model_id;
  if (!cloud_->GetName(&name, nullptr) ||
      !cloud_->GetModelId(&model_id, nullptr)) {
    return;
  }
  DCHECK_EQ(model_id.size(), 5U);

  VLOG(1) << "Starting peerd advertising.";
  const uint16_t port = device_->GetHttpEnpoint().first;
  DCHECK_NE(port, 0);

  std::string services;
  if (!cloud_->GetServices().empty())
    services += "_";
  services += Join(",_", cloud_->GetServices());

  std::vector<std::string> txt_record{
      {"txtvers=3"},
      {"ty=" + name},
      {"services=" + services},
      {"id=" + GetId()},
      {"mmid=" + model_id},
      {"flags=" + WifiSsidGenerator{cloud_, wifi_}.GenerateFlags()},
  };

  if (!cloud_->GetCloudId().empty())
    txt_record.emplace_back("gcd_id=" + cloud_->GetCloudId());

  if (!cloud_->GetDescription().empty())
    txt_record.emplace_back("note=" + cloud_->GetDescription());

  is_publishing_ = true;
  mdns_->PublishService(kPrivetServiceType, port, txt_record);
}

void Publisher::RemoveService() {
  if (!is_publishing_)
    return;
  is_publishing_ = false;
  VLOG(1) << "Stopping service publishing.";
  mdns_->StopPublishing(kPrivetServiceType);
}

}  // namespace privet
}  // namespace weave
