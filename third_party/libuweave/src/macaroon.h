// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_MACAROON_H_
#define LIBUWEAVE_SRC_MACAROON_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "macaroon_caveat.h"

#define UW_MACAROON_MAC_LEN 16

// Note: If we are looking to make memory savings on MCUs,
// at the cost of a little extra processing, we can make
// the macaroon encoding the actual in-memory representation.
// This can save much copying of macaroon data if need be.
typedef struct {
  uint8_t mac_tag[UW_MACAROON_MAC_LEN];
  size_t num_caveats;
  const UwMacaroonCaveat* caveats;
} UwMacaroon;

bool uw_macaroon_new_from_mac_tag_(UwMacaroon* new_macaroon,
                                   const uint8_t mac_tag[UW_MACAROON_MAC_LEN],
                                   const UwMacaroonCaveat* caveats,
                                   size_t num_caveats);

bool uw_macaroon_new_from_root_key_(UwMacaroon* new_macaroon,
                                    const uint8_t* root_key,
                                    size_t root_key_len,
                                    const UwMacaroonCaveat* caveats,
                                    size_t num_caveats);

bool uw_macaroon_verify_(const UwMacaroon* macaroon,
                         const uint8_t* root_key,
                         size_t root_key_len);

// Create a new macaroon with a new caveat
bool uw_macaroon_extend_(const UwMacaroon* old_macaroon,
                         UwMacaroon* new_macaroon,
                         const UwMacaroonCaveat* additional_caveat,
                         uint8_t* buffer, size_t buffer_size);

#endif  // LIBUWEAVE_SRC_MACAROON_H_
