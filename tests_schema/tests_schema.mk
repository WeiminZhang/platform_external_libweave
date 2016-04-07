# Copyright 2016 The Weave Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

###
# test_schema

TESTS_SCHEMA_DAEMON_SRC_FILES := \
	tests_schema/daemon/testdevice/testdevice.cc

tests_schema_daemon_obj_files := $(TESTS_SCHEMA_DAEMON_SRC_FILES:%.cc=out/$(BUILD_MODE)/%.o)

$(tests_schema_daemon_obj_files) : $(LIBEVHTP_HEADERS)
$(tests_schema_daemon_obj_files) : INCLUDES += $(LIBEVHTP_INCLUDES)
$(tests_schema_daemon_obj_files) : out/$(BUILD_MODE)/%.o : %.cc
	mkdir -p $(dir $@)
	$(CXX) $(DEFS_$(BUILD_MODE)) $(INCLUDES) $(CFLAGS) $(CFLAGS_$(BUILD_MODE)) $(CFLAGS_CC) -c -o $@ $<

tests_schema_daemon_common_flags := \
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

tests_schema_daemon_deps := out/$(BUILD_MODE)/examples_provider.a out/$(BUILD_MODE)/libweave.so

ifeq (1, $(USE_INTERNAL_LIBEVHTP))
tests_schema_daemon_deps += $(third_party_libevhtp_lib)
else
tests_schema_daemon_common_flags += -levhtp
endif

out/$(BUILD_MODE)/weave_daemon_testdevice : out/$(BUILD_MODE)/tests_schema/daemon/testdevice/testdevice.o $(tests_schema_daemon_deps)
	$(CXX) -o $@ $^ $(CFLAGS) $(tests_schema_daemon_common_flags)

all-testdevices : out/$(BUILD_MODE)/weave_daemon_testdevice

.PHONY : all-testdevices

