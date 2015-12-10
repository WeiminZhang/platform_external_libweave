// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/macaroon_caveat.h"

#include <string.h>

#include "src/crypto_hmac.h"
#include "src/macaroon_context.h"
#include "src/macaroon_encoding.h"

// TODO(bozhu): Find a better way to pre-allocate memory for HMACc computations?
// Are C99 variable-length arrays allowed on embedded devices?
#define HMAC_STATE_BUFFER_SIZE 1024

static bool create_caveat_(UwMacaroonCaveatType type, const void* value,
                           size_t value_len, uint8_t* buffer,
                           size_t buffer_size, UwMacaroonCaveat* caveat) {
  if (buffer == NULL || buffer_size == 0 || caveat == NULL) {
    // Here value can be NULL, and value_len can be 0
    return false;
  }

  caveat->bytes = buffer;
  size_t encoded_str_len, total_str_len;

  uint32_t unsigned_int = (uint32_t)type;
  if (!uw_macaroon_encoding_encode_uint_(unsigned_int, buffer, buffer_size,
                                         &encoded_str_len)) {
    return false;
  }
  total_str_len = encoded_str_len;
  buffer += encoded_str_len;
  buffer_size -= encoded_str_len;

  switch (type) {
    case kUwMacaroonCaveatTypeStop:
    case kUwMacaroonCaveatTypeSessionIdentifier:
      // No value
      encoded_str_len = 0;
      break;

    case kUwMacaroonCaveatTypeScope:
    case kUwMacaroonCaveatTypeIssued:
    case kUwMacaroonCaveatTypeTTL:
    case kUwMacaroonCaveatTypeExpiration:
      // Integer
      if (value_len != sizeof(uint32_t)) {
        // Wrong size for integers
        return false;
      }
      unsigned_int = *((uint32_t*)value);
      if (!uw_macaroon_encoding_encode_uint_(unsigned_int, buffer, buffer_size,
                                             &encoded_str_len)) {
        return false;
      }
      break;

    case kUwMacaroonCaveatTypeIdentifier:
      // Text string
      if (!uw_macaroon_encoding_encode_text_str_((uint8_t*)value, value_len,
                                                 buffer, buffer_size,
                                                 &encoded_str_len)) {
        return false;
      }
      break;

    default:
      // Should never reach here
      return false;
  }

  total_str_len += encoded_str_len;
  caveat->num_bytes = total_str_len;
  return true;
}

bool uw_macaroon_caveat_create_without_value_(UwMacaroonCaveatType type,
                                              uint8_t* buffer,
                                              size_t buffer_size,
                                              UwMacaroonCaveat* caveat) {
  if (buffer == NULL || buffer_size == 0 || caveat == NULL) {
    return false;
  }
  if (type != kUwMacaroonCaveatTypeStop &&
      type != kUwMacaroonCaveatTypeSessionIdentifier) {
    return false;
  }

  return create_caveat_(type, NULL, 0, buffer, buffer_size, caveat);
}

bool uw_macaroon_caveat_create_with_uint_(UwMacaroonCaveatType type,
                                          uint32_t value, uint8_t* buffer,
                                          size_t buffer_size,
                                          UwMacaroonCaveat* caveat) {
  if (buffer == NULL || buffer_size == 0 || caveat == NULL) {
    return false;
  }
  if (type != kUwMacaroonCaveatTypeScope &&
      type != kUwMacaroonCaveatTypeIssued &&
      type != kUwMacaroonCaveatTypeTTL &&
      type != kUwMacaroonCaveatTypeExpiration) {
    return false;
  }

  return create_caveat_(type, &value, sizeof(uint32_t), buffer, buffer_size,
                        caveat);
}

bool uw_macaroon_caveat_create_with_str_(UwMacaroonCaveatType type,
                                         const uint8_t* str, size_t str_len,
                                         uint8_t* buffer, size_t buffer_size,
                                         UwMacaroonCaveat* caveat) {
  if (buffer == NULL || buffer_size == 0 || caveat == NULL ||
      (str == NULL && str_len != 0)) {
    return false;
  }
  if (type != kUwMacaroonCaveatTypeIdentifier) {
    return false;
  }

  return create_caveat_(type, str, str_len, buffer, buffer_size, caveat);
}

bool uw_macaroon_caveat_get_type_(const UwMacaroonCaveat* caveat,
                                  UwMacaroonCaveatType* type) {
  if (caveat == NULL || type == NULL) {
    return false;
  }

  uint32_t unsigned_int;
  if (!uw_macaroon_encoding_decode_uint_(caveat->bytes, caveat->num_bytes,
                                         &unsigned_int)) {
    return false;
  }

  *type = (UwMacaroonCaveatType)unsigned_int;

  if (*type != kUwMacaroonCaveatTypeStop &&
      *type != kUwMacaroonCaveatTypeScope &&
      *type != kUwMacaroonCaveatTypeIdentifier &&
      *type != kUwMacaroonCaveatTypeIssued &&
      *type != kUwMacaroonCaveatTypeTTL &&
      *type != kUwMacaroonCaveatTypeExpiration &&
      *type != kUwMacaroonCaveatTypeSessionIdentifier) {
    return false;
  }

  return true;
}

bool uw_macaroon_caveat_get_value_uint_(const UwMacaroonCaveat* caveat,
                                        uint32_t* unsigned_int) {
  if (caveat == NULL || unsigned_int == NULL) {
    return false;
  }

  UwMacaroonCaveatType type;
  if (!uw_macaroon_caveat_get_type_(caveat, &type)) {
    return false;
  }
  if (type != kUwMacaroonCaveatTypeScope &&
      type != kUwMacaroonCaveatTypeIssued &&
      type != kUwMacaroonCaveatTypeTTL &&
      type != kUwMacaroonCaveatTypeExpiration) {
    // Wrong type
    return false;
  }

  size_t offset;
  if (!uw_macaroon_encoding_get_item_len_(caveat->bytes, caveat->num_bytes,
                                          &offset)) {
    return false;
  }

  return uw_macaroon_encoding_decode_uint_(
      caveat->bytes + offset, caveat->num_bytes - offset, unsigned_int);
}

bool uw_macaroon_caveat_get_value_str_(const UwMacaroonCaveat* caveat,
                                       const uint8_t** str, size_t* str_len) {
  if (caveat == NULL || str == NULL || str_len == NULL) {
    return false;
  }

  UwMacaroonCaveatType type;
  if (!uw_macaroon_caveat_get_type_(caveat, &type)) {
    return false;
  }
  if (type != kUwMacaroonCaveatTypeIdentifier) {
    // Wrong type
    return false;
  }

  size_t offset;
  if (!uw_macaroon_encoding_get_item_len_(caveat->bytes, caveat->num_bytes,
                                          &offset)) {
    return false;
  }

  return uw_macaroon_encoding_decode_text_str_(
      caveat->bytes + offset, caveat->num_bytes - offset, str, str_len);
}

bool uw_macaroon_caveat_sign_(const uint8_t* key, size_t key_len,
                              const UwMacaroonCaveat* caveat, uint8_t* mac_tag,
                              size_t mac_tag_size) {
  if (key == NULL || key_len == 0 || caveat == NULL || mac_tag == NULL ||
      mac_tag_size == 0) {
    return false;
  }

  uint8_t hmac_state_buffer[HMAC_STATE_BUFFER_SIZE];
  if (HMAC_STATE_BUFFER_SIZE < uw_crypto_hmac_required_buffer_size_()) {
    return false;
  }

  if (!uw_crypto_hmac_init_(hmac_state_buffer, HMAC_STATE_BUFFER_SIZE, key,
                            key_len)) {
    return false;
  }

  if (!uw_crypto_hmac_update_(hmac_state_buffer, HMAC_STATE_BUFFER_SIZE,
                              caveat->bytes, caveat->num_bytes)) {
    return false;
  }

  const uint8_t* context;
  size_t context_len;
  UwMacaroonCaveatType caveat_type;

  if ((!uw_macaroon_caveat_get_type_(caveat, &caveat_type)) ||
      (!uw_macaroon_context_get_(caveat_type, &context, &context_len))) {
    return false;
  }
  if (context != NULL && context_len != 0) {
    if (!uw_crypto_hmac_update_(hmac_state_buffer, HMAC_STATE_BUFFER_SIZE,
                                context, context_len)) {
      return false;
    }
  }

  return uw_crypto_hmac_final_(hmac_state_buffer, HMAC_STATE_BUFFER_SIZE,
                               mac_tag, mac_tag_size);
}
