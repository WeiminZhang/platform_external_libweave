// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/macaroon.h"

#include <string.h>

#include "src/crypto_utils.h"

static bool create_mac_tag_(const uint8_t* key, size_t key_len,
                            const UwMacaroonCaveat* caveats, size_t num_caveats,
                            uint8_t mac_tag[UW_MACAROON_MAC_LEN]) {
  if (key == NULL || key_len == 0 || caveats == NULL || num_caveats == 0 ||
      mac_tag == NULL) {
    return false;
  }

  // Store the intermediate MAC tags in an internal buffer before we finish the
  // whole computation.
  // If we use the output buffer mac_tag directly and certain errors happen in
  // the middle of this computation, mac_tag will probably contain a valid
  // macaroon tag with large scope than expected.
  uint8_t mac_tag_buff[UW_MACAROON_MAC_LEN];

  // Compute the first tag by using the key
  if (!uw_macaroon_caveat_sign_(key, key_len, &(caveats[0]), mac_tag_buff,
                                UW_MACAROON_MAC_LEN)) {
    return false;
  }

  // Compute the rest of the tags by using the tag as the key
  for (size_t i = 1; i < num_caveats; i++) {
    if (!uw_macaroon_caveat_sign_(mac_tag_buff, UW_MACAROON_MAC_LEN,
                                  &(caveats[i]), mac_tag_buff,
                                  UW_MACAROON_MAC_LEN)) {
      return false;
    }
  }

  memcpy(mac_tag, mac_tag_buff, UW_MACAROON_MAC_LEN);
  return true;
}

bool uw_macaroon_new_from_mac_tag_(UwMacaroon* new_macaroon,
                                   const uint8_t mac_tag[UW_MACAROON_MAC_LEN],
                                   const UwMacaroonCaveat* caveats,
                                   size_t num_caveats) {
  if (new_macaroon == NULL || mac_tag == NULL || caveats == NULL ||
      num_caveats == 0) {
    return false;
  }

  memcpy(new_macaroon->mac_tag, mac_tag, UW_MACAROON_MAC_LEN);
  new_macaroon->num_caveats = num_caveats;
  new_macaroon->caveats = caveats;

  return true;
}

bool uw_macaroon_new_from_root_key_(UwMacaroon* new_macaroon,
                                    const uint8_t* root_key,
                                    size_t root_key_len,
                                    const UwMacaroonCaveat* caveats,
                                    size_t num_caveats) {
  if (new_macaroon == NULL || root_key == NULL || root_key_len == 0 ||
      caveats == NULL || num_caveats == 0) {
    return false;
  }

  if (!create_mac_tag_(root_key, root_key_len, caveats, num_caveats,
                       new_macaroon->mac_tag)) {
    return false;
  }

  new_macaroon->num_caveats = num_caveats;
  new_macaroon->caveats = caveats;

  return true;
}

bool uw_macaroon_verify_(const UwMacaroon* macaroon,
                         const uint8_t* root_key,
                         size_t root_key_len) {
  if (macaroon == NULL || root_key == NULL) {
    return false;
  }

  uint8_t mac_tag[UW_MACAROON_MAC_LEN] = {0};
  if (!create_mac_tag_(root_key, root_key_len, macaroon->caveats,
                       macaroon->num_caveats, mac_tag)) {
    return false;
  }

  return uw_crypto_utils_equal_(mac_tag, macaroon->mac_tag,
                                UW_MACAROON_MAC_LEN);
}

bool uw_macaroon_extend_(const UwMacaroon* old_macaroon,
                         UwMacaroon* new_macaroon,
                         const UwMacaroonCaveat* additional_caveat,
                         uint8_t* buffer, size_t buffer_size) {
  if (old_macaroon == NULL || new_macaroon == NULL ||
      additional_caveat == NULL || buffer == NULL || buffer_size == 0) {
    return false;
  }

  new_macaroon->num_caveats = old_macaroon->num_caveats + 1;

  // Extend the caveat list
  if ((new_macaroon->num_caveats) * sizeof(UwMacaroonCaveat) > buffer_size) {
    // Not enough memory to store the extended caveat list
    return false;
  }
  UwMacaroonCaveat* extended_list = (UwMacaroonCaveat*)buffer;
  if (old_macaroon->caveats != NULL && extended_list != old_macaroon->caveats) {
    memcpy(extended_list, old_macaroon->caveats,
           (old_macaroon->num_caveats) * sizeof(UwMacaroonCaveat));
  }
  extended_list[old_macaroon->num_caveats] = *additional_caveat;
  new_macaroon->caveats = extended_list;

  // Compute the new MAC tag
  return create_mac_tag_(old_macaroon->mac_tag, UW_MACAROON_MAC_LEN,
                         additional_caveat, 1, new_macaroon->mac_tag);
}
