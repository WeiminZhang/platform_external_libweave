// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "libweave/examples/ubuntu/avahi_client.h"

#include <cstdlib>
#include <vector>

#include <avahi-common/error.h>

namespace weave {
namespace examples {

namespace {

void GroupCallback(AvahiEntryGroup* g,
                   AvahiEntryGroupState state,
                   AVAHI_GCC_UNUSED void* userdata) {
  CHECK_NE(state, AVAHI_ENTRY_GROUP_COLLISION);
  CHECK_NE(state, AVAHI_ENTRY_GROUP_FAILURE);
}

}  // namespace

MdnsImpl::MdnsImpl() {
  CHECK_EQ(0, std::system("service avahi-daemon status | grep running || "
                          "service avahi-daemon start"));
  thread_pool_.reset(avahi_threaded_poll_new());
  CHECK(thread_pool_);

  int ret = 0;
  client_.reset(avahi_client_new(avahi_threaded_poll_get(thread_pool_.get()),
                                 {}, nullptr, this, &ret));
  CHECK(client_) << avahi_strerror(ret);

  avahi_threaded_poll_start(thread_pool_.get());

  group_.reset(avahi_entry_group_new(client_.get(), GroupCallback, nullptr));
  CHECK(group_) << avahi_strerror(avahi_client_errno(client_.get()))
                << ". Check avahi-daemon configuration";
}

MdnsImpl::~MdnsImpl() {
  if (thread_pool_)
    avahi_threaded_poll_stop(thread_pool_.get());
}

void MdnsImpl::PublishService(const std::string& service_type,
                              uint16_t port,
                              const std::vector<std::string>& txt) {
  LOG(INFO) << "Publishing service";
  CHECK(group_);

  // Create txt record.
  std::unique_ptr<AvahiStringList, decltype(&avahi_string_list_free)> txt_list{
      nullptr, &avahi_string_list_free};
  if (!txt.empty()) {
    std::vector<const char*> txt_vector_ptr;
    for (const auto& i : txt)
      txt_vector_ptr.push_back(i.c_str());
    txt_list.reset(avahi_string_list_new_from_array(txt_vector_ptr.data(),
                                                    txt_vector_ptr.size()));
    CHECK(txt_list);
  }

  int ret = 0;
  if (prev_port_ == port && prev_type_ == service_type) {
    ret = avahi_entry_group_update_service_txt_strlst(
        group_.get(), AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, {}, GetId().c_str(),
        service_type.c_str(), nullptr, txt_list.get());
    CHECK_GE(ret, 0) << avahi_strerror(ret);
  } else {
    prev_port_ = port;
    prev_type_ = service_type;

    avahi_entry_group_reset(group_.get());
    CHECK(avahi_entry_group_is_empty(group_.get()));

    ret = avahi_entry_group_add_service_strlst(
        group_.get(), AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, {}, GetId().c_str(),
        service_type.c_str(), nullptr, nullptr, port, txt_list.get());
    CHECK_GE(ret, 0) << avahi_strerror(ret);
    ret = avahi_entry_group_commit(group_.get());
    CHECK_GE(ret, 0) << avahi_strerror(ret);
  }
}

void MdnsImpl::StopPublishing(const std::string& service_name) {
  CHECK(group_);
  avahi_entry_group_reset(group_.get());
}

std::string MdnsImpl::GetId() const {
  return "WEAVE" + std::to_string(gethostid());
}

}  // namespace examples
}  // namespace weave
