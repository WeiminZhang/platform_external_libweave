// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_EXAMPLES_PROVIDER_EVENT_HTTP_CLIENT_H_
#define LIBWEAVE_EXAMPLES_PROVIDER_EVENT_HTTP_CLIENT_H_

#include <string>

#include <base/memory/weak_ptr.h>
#include <weave/provider/http_client.h>

#include "examples/provider/event_task_runner.h"

namespace weave {
namespace examples {

// Basic implementation of weave::HttpClient using libevent.
class EventHttpClient : public provider::HttpClient {
 public:
  explicit EventHttpClient(EventTaskRunner* task_runner);

  void SendRequest(Method method,
                   const std::string& url,
                   const Headers& headers,
                   const std::string& data,
                   const SendRequestCallback& callback) override;

 private:
  EventTaskRunner* task_runner_{nullptr};

  base::WeakPtrFactory<EventHttpClient> weak_ptr_factory_{this};
};

}  // namespace examples
}  // namespace weave

#endif  // LIBWEAVE_EXAMPLES_PROVIDER_EVENT_HTTP_CLIENT_H_
