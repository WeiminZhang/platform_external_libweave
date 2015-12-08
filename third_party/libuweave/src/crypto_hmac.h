// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_CRYPTO_HMAC_H_
#define LIBUWEAVE_SRC_CRYPTO_HMAC_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Return the minimum required number of bytes for the state_buffer used in the
// init, update and final functions.
size_t uw_crypto_hmac_required_buffer_size_();

bool uw_crypto_hmac_init_(uint8_t* state_buffer,
                          size_t state_buffer_len,
                          const uint8_t* key,
                          size_t key_len);
bool uw_crypto_hmac_update_(uint8_t* state_buffer,
                            size_t state_buffer_len,
                            const uint8_t* data,
                            size_t data_len);
bool uw_crypto_hmac_final_(uint8_t* state_buffer,
                           size_t state_buffer_len,
                           uint8_t* truncated_digest,
                           size_t truncated_digest_len);

#endif  // LIBUWEAVE_SRC_CRYPTO_HMAC_H_
