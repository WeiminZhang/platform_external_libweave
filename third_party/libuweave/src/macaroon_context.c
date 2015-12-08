// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/macaroon_context.h"

#include "src/macaroon_caveat.h"

bool uw_macaroon_context_get_(UwMacaroonCaveatType type,
                              const uint8_t** context, size_t* context_len) {
  if (type != kUwMacaroonCaveatTypeSessionIdentifier) {
    *context = NULL;
    *context_len = 0;
  }

  // TODO(bozhu): Waiting for a proper way to obtain the session identifier.
  // Have we already implemented something related to session identifiers?
  *context = NULL;
  *context_len = 0;

  return true;
}
