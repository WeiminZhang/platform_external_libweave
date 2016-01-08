# Copyright 2015 The Weave Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

###
# examples

examples_provider_obj_files := $(EXAMPLES_PROVIDER_SRC_FILES:%.cc=out/$(BUILD_MODE)/%.o)

# We don't need libevent.a, but the headers files in third_party/include.
$(examples_provider_obj_files) : out/$(BUILD_MODE)/%.o : %.cc third_party/lib/libevent.a
	mkdir -p $(dir $@)
	$(CXX) $(DEFS_$(BUILD_MODE)) $(INCLUDES) $(CFLAGS) $(CFLAGS_$(BUILD_MODE)) $(CFLAGS_CC) -c -o $@ $<

out/$(BUILD_MODE)/examples_provider.a : $(examples_provider_obj_files)
	rm -f $@
	$(AR) crsT $@ $^

# We don't need libevent.a, but the headers files in third_party/include.
out/$(BUILD_MODE)/examples/daemon/%.o : examples/daemon/%.cc third_party/lib/libevent.a
	mkdir -p $(dir $@)
	$(CXX) $(DEFS_$(BUILD_MODE)) $(INCLUDES) $(CFLAGS) $(CFLAGS_$(BUILD_MODE)) $(CFLAGS_CC) -c -o $@ $<

daemon_common_flags := \
	-Wl,-rpath=out/$(BUILD_MODE)/ \
	-Lthird_party/lib \
	-levent \
	-levent_openssl \
	-lpthread \
	-lavahi-common \
	-lavahi-client \
	-lexpat \
	-lcurl \
	-lssl \
	-lcrypto

out/$(BUILD_MODE)/weave_daemon_ledflasher : out/$(BUILD_MODE)/examples/daemon/ledflasher/ledflasher.o out/$(BUILD_MODE)/examples_provider.a out/$(BUILD_MODE)/libweave.so
	$(CXX) -o $@ $^ $(daemon_common_flags)

out/$(BUILD_MODE)/weave_daemon_light : out/$(BUILD_MODE)/examples/daemon/light/light.o out/$(BUILD_MODE)/examples_provider.a out/$(BUILD_MODE)/libweave.so
	$(CXX) -o $@ $^ $(daemon_common_flags)

out/$(BUILD_MODE)/weave_daemon_lock : out/$(BUILD_MODE)/examples/daemon/lock/lock.o out/$(BUILD_MODE)/examples_provider.a out/$(BUILD_MODE)/libweave.so
	$(CXX) -o $@ $^ $(daemon_common_flags)

out/$(BUILD_MODE)/weave_daemon_oven : out/$(BUILD_MODE)/examples/daemon/oven/oven.o out/$(BUILD_MODE)/examples_provider.a out/$(BUILD_MODE)/libweave.so
	$(CXX) -o $@ $^ $(daemon_common_flags)

out/$(BUILD_MODE)/weave_daemon_sample : out/$(BUILD_MODE)/examples/daemon/sample/sample.o out/$(BUILD_MODE)/examples_provider.a out/$(BUILD_MODE)/libweave.so
	$(CXX) -o $@ $^ $(daemon_common_flags)

out/$(BUILD_MODE)/weave_daemon_speaker : out/$(BUILD_MODE)/examples/daemon/speaker/speaker.o out/$(BUILD_MODE)/examples_provider.a out/$(BUILD_MODE)/libweave.so
	$(CXX) -o $@ $^ $(daemon_common_flags)

all-examples : out/$(BUILD_MODE)/weave_daemon_ledflasher out/$(BUILD_MODE)/weave_daemon_light out/$(BUILD_MODE)/weave_daemon_lock out/$(BUILD_MODE)/weave_daemon_oven out/$(BUILD_MODE)/weave_daemon_sample out/$(BUILD_MODE)/weave_daemon_speaker

.PHONY : all-examples

