// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_MACAROON_CAVEAT_H_
#define LIBUWEAVE_SRC_MACAROON_CAVEAT_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
  size_t num_bytes;
  const uint8_t* bytes;
} UwMacaroonCaveat;

typedef enum {
  kUwMacaroonCaveatTypeStop = 0,
  kUwMacaroonCaveatTypeScope = 1,
  kUwMacaroonCaveatTypeIdentifier = 2,
  kUwMacaroonCaveatTypeIssued = 3,
  kUwMacaroonCaveatTypeTTL = 4,
  kUwMacaroonCaveatTypeExpiration = 5,
  kUwMacaroonCaveatTypeSessionIdentifier = 16
} UwMacaroonCaveatType;

bool uw_macaroon_caveat_create_without_value_(UwMacaroonCaveatType type,
                                              uint8_t* buffer,
                                              size_t buffer_size,
                                              UwMacaroonCaveat* new_caveat);
bool uw_macaroon_caveat_create_with_uint_(UwMacaroonCaveatType type,
                                          uint32_t value, uint8_t* buffer,
                                          size_t buffer_size,
                                          UwMacaroonCaveat* new_caveat);
bool uw_macaroon_caveat_create_with_str_(UwMacaroonCaveatType type,
                                         const uint8_t* str, size_t str_len,
                                         uint8_t* buffer, size_t buffer_size,
                                         UwMacaroonCaveat* new_caveat);

bool uw_macaroon_caveat_get_type_(const UwMacaroonCaveat* caveat,
                                  UwMacaroonCaveatType* type);
bool uw_macaroon_caveat_get_value_uint_(const UwMacaroonCaveat* caveat,
                                        uint32_t* unsigned_int);
bool uw_macaroon_caveat_get_value_str_(const UwMacaroonCaveat* caveat,
                                       const uint8_t** str, size_t* str_len);

bool uw_macaroon_caveat_sign_(const uint8_t* key, size_t key_len,
                              const UwMacaroonCaveat* caveat, uint8_t* mac_tag,
                              size_t mac_tag_size);

#endif  // LIBUWEAVE_SRC_MACAROON_CAVEAT_H_
