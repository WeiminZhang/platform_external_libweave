// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_PRIVET_H_
#define LIBWEAVE_INCLUDE_WEAVE_PRIVET_H_

#include <string>
#include <vector>

#include <base/callback.h>
#include <weave/settings.h>

namespace weave {

class Privet {
 public:
  using OnPairingStartedCallback =
      base::Callback<void(const std::string& session_id,
                          PairingType pairing_type,
                          const std::vector<uint8_t>& code)>;
  using OnPairingEndedCallback =
      base::Callback<void(const std::string& session_id)>;

  virtual void AddOnPairingChangedCallbacks(
      const OnPairingStartedCallback& on_start,
      const OnPairingEndedCallback& on_end) = 0;

 protected:
  virtual ~Privet() = default;
};

}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_PRIVET_H_
