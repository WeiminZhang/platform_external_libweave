// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_STATE_H_
#define LIBWEAVE_INCLUDE_WEAVE_STATE_H_

#include <base/callback.h>
#include <base/values.h>

namespace weave {

class State {
 public:
  // Sets callback which is called when stat is changed.
  virtual void AddOnChangedCallback(const base::Closure& callback) = 0;

  // Returns value of the single property.
  // |name| is full property name, including package name. e.g. "base.network".
  virtual std::unique_ptr<base::Value> GetStateProperty(
      const std::string& name) = 0;

  // Sets value of the single property.
  // |name| is full property name, including package name. e.g. "base.network".
  virtual bool SetStateProperty(const std::string& name,
                                const base::Value& value,
                                ErrorPtr* error) = 0;

  // Updates multiple property values.
  virtual bool SetProperties(const base::DictionaryValue& property_set,
                             ErrorPtr* error) = 0;

  // Returns aggregated state properties across all registered packages.
  virtual std::unique_ptr<base::DictionaryValue> GetState() const = 0;

 protected:
  virtual ~State() = default;
};

}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_STATE_H_
