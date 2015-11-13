// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_EXAMPLES_PROVIDER_EVENT_DELETER_H
#define LIBWEAVE_EXAMPLES_PROVIDER_EVENT_DELETER_H

#include <memory>

#include <third_party/include/event2/event.h>
#include <third_party/include/event2/event_struct.h>
#include <third_party/include/event2/http.h>

namespace weave {
namespace examples {

// Defines overloaded deletion methods for various event_ objects
// so we can use one unique_ptr definition for all of them
class EventDeleter {
 public:
  void operator()(evhttp_uri* http_uri) { evhttp_uri_free(http_uri); }
  void operator()(evhttp_connection* conn) { evhttp_connection_free(conn); }
  void operator()(evhttp_request* req) { evhttp_request_free(req); }
  void operator()(event_base* base) { event_base_free(base); }
  void operator()(event* ev) {
    event_del(ev);
    event_free(ev);
  }
};

template <typename T>
using EventPtr = std::unique_ptr<T, EventDeleter>;

}  // namespace examples
}  // namespace weave

#endif  // LIBWEAVE_EXAMPLES_PROVIDER_EVENT_DELETER_H
