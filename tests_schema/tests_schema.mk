# Copyright 2016 The Weave Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

###
# examples

tests_schema_provider_obj_files := $(EXAMPLES_PROVIDER_SRC_FILES:%.cc=out/$(BUILD_MODE)/%.o)

USE_INTERNAL_LIBEVHTP ?= 1

ifeq (1, $(USE_INTERNAL_LIBEVHTP))
LIBEVHTP_INCLUDES = -Ithird_party/libevhtp -I$(dir $(third_party_libevhtp_header))
LIBEVHTP_HEADERS = $(third_party_libevhtp_header)
else
LIBEVHTP_INCLUDES =
LIBEVHTP_HEADERS =
endif

$(tests_schema_provider_obj_files) : $(LIBEVHTP_HEADERS)
$(tests_schema_provider_obj_files) : INCLUDES += $(LIBEVHTP_INCLUDES)
$(tests_schema_provider_obj_files) : out/$(BUILD_MODE)/%.o : %.cc
	mkdir -p $(dir $@)
	$(CXX) $(DEFS_$(BUILD_MODE)) $(INCLUDES) $(CFLAGS) $(CFLAGS_$(BUILD_MODE)) $(CFLAGS_CC) -c -o $@ $<

out/$(BUILD_MODE)/examples_provider.a : $(tests_schema_provider_obj_files)
	rm -f $@
	$(AR) crsT $@ $^

TESTS_SCHEMA_DAEMON_SRC_FILES := \
	tests_schema/daemon/testdevice/testdevice.cc

tests_schema_daemon_obj_files := $(TESTS_SCHEMA_DAEMON_SRC_FILES:%.cc=out/$(BUILD_MODE)/%.o)

$(tests_schema_daemon_obj_files) : $(LIBEVHTP_HEADERS)
$(tests_schema_daemon_obj_files) : INCLUDES += $(LIBEVHTP_INCLUDES)
$(tests_schema_daemon_obj_files) : out/$(BUILD_MODE)/%.o : %.cc
	mkdir -p $(dir $@)
	$(CXX) $(DEFS_$(BUILD_MODE)) $(INCLUDES) $(CFLAGS) $(CFLAGS_$(BUILD_MODE)) $(CFLAGS_CC) -c -o $@ $<

daemon_common_flags := \
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

daemon_deps := out/$(BUILD_MODE)/examples_provider.a out/$(BUILD_MODE)/libweave.so

ifeq (1, $(USE_INTERNAL_LIBEVHTP))
daemon_deps += $(third_party_libevhtp_lib)
else
daemon_common_flags += -levhtp
endif

out/$(BUILD_MODE)/weave_daemon_testdevice : out/$(BUILD_MODE)/tests_schema/daemon/testdevice/testdevice.o $(daemon_deps)
	$(CXX) -o $@ $^ $(CFLAGS) $(daemon_common_flags)

all-testdevices : out/$(BUILD_MODE)/weave_daemon_testdevice

.PHONY : all-testdevices

