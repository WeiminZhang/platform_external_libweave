// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/privet/openssl_utils.h"

#include <algorithm>

#include <base/logging.h>

extern "C" {
#include "third_party/libuweave/src/crypto_hmac.h"
}

namespace weave {
namespace privet {

std::vector<uint8_t> HmacSha256(const std::vector<uint8_t>& key,
                                const std::vector<uint8_t>& data) {
  std::vector<uint8_t> mac(kSha256OutputSize);
  uint8_t hmac_state[uw_crypto_hmac_required_buffer_size_()];
  CHECK_EQ(0u, uw_crypto_hmac_init_(hmac_state, sizeof(hmac_state), key.data(),
                                    key.size()));
  CHECK(uw_crypto_hmac_update_(hmac_state, sizeof(hmac_state), data.data(),
                               data.size()));
  CHECK(uw_crypto_hmac_final_(hmac_state, sizeof(hmac_state), mac.data(),
                              mac.size()));
  return mac;
}

}  // namespace privet
}  // namespace weave
