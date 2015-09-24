// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_DNS_SERVICE_DISCOVERY_PROVIDER_H_
#define LIBWEAVE_INCLUDE_WEAVE_DNS_SERVICE_DISCOVERY_PROVIDER_H_

#include <string>
#include <vector>

#include <base/callback.h>

namespace weave {

class DnsServiceDiscoveryProvider {
 public:
  // Publishes new service using DNS-SD or updates existing one.
  virtual void PublishService(const std::string& service_type,
                              uint16_t port,
                              const std::vector<std::string>& txt) = 0;

  // Stops publishing service.
  virtual void StopPublishing(const std::string& service_type) = 0;

  // Returns permanent device ID.
  // TODO(vitalybuka): Find better place for this information.
  virtual std::string GetId() const = 0;

 protected:
  virtual ~DnsServiceDiscoveryProvider() = default;
};

}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_DNS_SERVICE_DISCOVERY_PROVIDER_H_
