// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_TEST_MOCK_DNS_SERVICE_DISCOVERY_PROVIDER_H_
#define LIBWEAVE_INCLUDE_WEAVE_TEST_MOCK_DNS_SERVICE_DISCOVERY_PROVIDER_H_

#include <weave/dns_service_discovery_provider.h>

#include <string>
#include <vector>

#include <gmock/gmock.h>

namespace weave {
namespace test {

class MockDnsServiceDiscovery : public DnsServiceDiscoveryProvider {
 public:
  MOCK_METHOD3(PublishService,
               void(const std::string&,
                    uint16_t,
                    const std::vector<std::string>&));
  MOCK_METHOD1(StopPublishing, void(const std::string&));
  MOCK_CONST_METHOD0(GetId, std::string());
};

}  // namespace test
}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_TEST_MOCK_DNS_SERVICE_DISCOVERY_PROVIDER_H_
