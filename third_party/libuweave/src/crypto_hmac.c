// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/crypto_hmac.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <openssl/evp.h>
#include <openssl/hmac.h>

size_t uw_crypto_hmac_required_buffer_size_() {
  return sizeof(HMAC_CTX);
}

bool uw_crypto_hmac_init_(uint8_t* state_buffer,
                          size_t state_buffer_len,
                          const uint8_t* key,
                          size_t key_len) {
  if (sizeof(HMAC_CTX) > state_buffer_len) {
    return false;
  }
  HMAC_CTX* context = (HMAC_CTX*)state_buffer;
  HMAC_CTX_init(context);
  return HMAC_Init(context, key, key_len, EVP_sha256());
}

bool uw_crypto_hmac_update_(uint8_t* state_buffer,
                            size_t state_buffer_len,
                            const uint8_t* data,
                            size_t data_len) {
  if (sizeof(HMAC_CTX) > state_buffer_len) {
    return false;
  }
  HMAC_CTX* context = (HMAC_CTX*)state_buffer;
  return HMAC_Update(context, data, data_len);
}

bool uw_crypto_hmac_final_(uint8_t* state_buffer,
                           size_t state_buffer_len,
                           uint8_t* truncated_digest,
                           size_t truncated_digest_len) {
  if (sizeof(HMAC_CTX) > state_buffer_len) {
    return false;
  }
  HMAC_CTX* context = (HMAC_CTX*)state_buffer;

  const size_t kFullDigestLen = (size_t)EVP_MD_size(EVP_sha256());
  if (truncated_digest_len > kFullDigestLen) {
    return false;
  }

  uint8_t digest[kFullDigestLen];
  uint32_t len = kFullDigestLen;

  bool result = HMAC_Final(context, digest, &len) && kFullDigestLen == len;
  HMAC_CTX_cleanup(context);
  if (result) {
    memcpy(truncated_digest, digest, truncated_digest_len);
  }
  return result;
}
