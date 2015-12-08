// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UW_LIBUWEAVE_SRC_MACAROON_CONTEXT_
#define UW_LIBUWEAVE_SRC_MACAROON_CONTEXT_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "src/macaroon_caveat.h"

bool uw_macaroon_context_get_(UwMacaroonCaveatType type,
                              const uint8_t** context, size_t* context_len);

#endif  // UW_LIBUWEAVE_SRC_MACAROON_CONTEXT_
