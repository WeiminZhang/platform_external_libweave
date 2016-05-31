# Copyright 2015 The Weave Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

###
# examples

examples_provider_obj_files := $(EXAMPLES_PROVIDER_SRC_FILES:%.cc=out/$(BUILD_MODE)/%.o)

$(examples_provider_obj_files) : $(LIBEVHTP_HEADERS)
$(examples_provider_obj_files) : INCLUDES += $(LIBEVHTP_INCLUDES)
$(examples_provider_obj_files) : out/$(BUILD_MODE)/%.o : %.cc
	mkdir -p $(dir $@)
	$(CXX) $(DEFS_$(BUILD_MODE)) $(INCLUDES) $(CFLAGS) $(CFLAGS_$(BUILD_MODE)) $(CFLAGS_CC) -c -o $@ $<

out/$(BUILD_MODE)/examples_provider.a : $(examples_provider_obj_files)
	rm -f $@
	$(AR) crsT $@ $^

EXAMPLES_DAEMON_SRC_FILES := \
	examples/daemon/ledflasher/ledflasher.cc \
	examples/daemon/light/light.cc \
	examples/daemon/lock/lock.cc \
	examples/daemon/sample/sample.cc

examples_daemon_obj_files := $(EXAMPLES_DAEMON_SRC_FILES:%.cc=out/$(BUILD_MODE)/%.o)

$(examples_daemon_obj_files) : $(LIBEVHTP_HEADERS)
$(examples_daemon_obj_files) : INCLUDES += $(LIBEVHTP_INCLUDES)
$(examples_daemon_obj_files) : out/$(BUILD_MODE)/%.o : %.cc
	mkdir -p $(dir $@)
	$(CXX) $(DEFS_$(BUILD_MODE)) $(INCLUDES) $(CFLAGS) $(CFLAGS_$(BUILD_MODE)) $(CFLAGS_CC) -c -o $@ $<

example_daemon_common_flags := \
	-Wl,-rpath=out/$(BUILD_MODE)/ \
	-levent \
	-levent_openssl \
	-lpthread \
	-lavahi-common \
	-lavahi-client \
	-lexpat \
	-lcurl \
	-lssl \
	-lcrypto

example_daemon_deps := out/$(BUILD_MODE)/examples_provider.a out/$(BUILD_MODE)/libweave.so

ifeq (1, $(USE_INTERNAL_LIBEVHTP))
example_daemon_deps += $(third_party_libevhtp_lib)
else
example_daemon_common_flags += -levhtp
endif

out/$(BUILD_MODE)/weave_daemon_ledflasher : out/$(BUILD_MODE)/examples/daemon/ledflasher/ledflasher.o $(example_daemon_deps)
	$(CXX) -o $@ $^ $(CFLAGS) $(example_daemon_common_flags)

out/$(BUILD_MODE)/weave_daemon_light : out/$(BUILD_MODE)/examples/daemon/light/light.o $(example_daemon_deps)
	$(CXX) -o $@ $^ $(CFLAGS) $(example_daemon_common_flags)

out/$(BUILD_MODE)/weave_daemon_lock : out/$(BUILD_MODE)/examples/daemon/lock/lock.o $(example_daemon_deps)
	$(CXX) -o $@ $^ $(CFLAGS) $(example_daemon_common_flags)

out/$(BUILD_MODE)/weave_daemon_sample : out/$(BUILD_MODE)/examples/daemon/sample/sample.o $(example_daemon_deps)
	$(CXX) -o $@ $^ $(CFLAGS) $(example_daemon_common_flags)

all-examples : out/$(BUILD_MODE)/weave_daemon_ledflasher out/$(BUILD_MODE)/weave_daemon_light out/$(BUILD_MODE)/weave_daemon_lock out/$(BUILD_MODE)/weave_daemon_sample

.PHONY : all-examples

