# Copyright 2015 The Weave Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

###
# third_party/chromium/

third_party_chromium_base_obj_files := $(THIRD_PARTY_CHROMIUM_BASE_SRC_FILES:%.cc=out/$(BUILD_MODE)/%.o)

$(third_party_chromium_base_obj_files) : out/$(BUILD_MODE)/%.o : %.cc
	mkdir -p $(dir $@)
	$(CXX) $(DEFS_$(BUILD_MODE)) $(INCLUDES) $(CFLAGS) $(CFLAGS_$(BUILD_MODE)) $(CFLAGS_CC) -c -o $@ $<

third_party_chromium_base_unittest_obj_files := $(THIRD_PARTY_CHROMIUM_BASE_UNITTEST_SRC_FILES:%.cc=out/$(BUILD_MODE)/%.o)

$(third_party_chromium_base_unittest_obj_files) : out/$(BUILD_MODE)/%.o : %.cc
	mkdir -p $(dir $@)
	$(CXX) $(DEFS_TEST) $(INCLUDES) $(CFLAGS) $(CFLAGS_$(BUILD_MODE)) $(CFLAGS_CC) -c -o $@ $<

third_party_chromium_crypto_obj_files := $(THIRD_PARTY_CHROMIUM_CRYPTO_SRC_FILES:%.cc=out/$(BUILD_MODE)/%.o)

$(third_party_chromium_crypto_obj_files) : out/$(BUILD_MODE)/%.o : %.cc
	mkdir -p $(dir $@)
	$(CXX) $(DEFS_$(BUILD_MODE)) $(INCLUDES) $(CFLAGS) $(CFLAGS_$(BUILD_MODE)) $(CFLAGS_CC) -c -o $@ $<

third_party_chromium_crypto_unittest_obj_files := $(THIRD_PARTY_CHROMIUM_CRYPTO_UNITTEST_SRC_FILES:%.cc=out/$(BUILD_MODE)/%.o)

$(third_party_chromium_crypto_unittest_obj_files) : out/$(BUILD_MODE)/%.o : %.cc
	mkdir -p $(dir $@)
	$(CXX) $(DEFS_TEST) $(INCLUDES) $(CFLAGS) $(CFLAGS_$(BUILD_MODE)) $(CFLAGS_CC) -c -o $@ $<

###
# third_party/modp_b64/

third_party_modp_b64_obj_files := $(THIRD_PARTY_MODP_B64_SRC_FILES:%.cc=out/$(BUILD_MODE)/%.o)

$(third_party_modp_b64_obj_files) : out/$(BUILD_MODE)/%.o : %.cc
	mkdir -p $(dir $@)
	$(CXX) $(DEFS_$(BUILD_MODE)) $(INCLUDES) $(CFLAGS) $(CFLAGS_$(BUILD_MODE)) $(CFLAGS_CC) -c -o $@ $<

###
# third_party/libuweave/

third_party_libuweave_obj_files := $(THIRD_PARTY_LIBUWEAVE_SRC_FILES:%.c=out/$(BUILD_MODE)/%.o)

$(third_party_libuweave_obj_files) : out/$(BUILD_MODE)/%.o : %.c
	mkdir -p $(dir $@)
	$(CC) $(DEFS_$(BUILD_MODE)) $(INCLUDES) $(CFLAGS) $(CFLAGS_$(BUILD_MODE)) $(CFLAGS_C) -c -o $@ $<

###
# libgtest and libgmock (third_party)

third_party_gtest_lib = out/$(BUILD_MODE)/third_party/googletest/libgtest.a
third_party_gmock_lib = out/$(BUILD_MODE)/third_party/googletest/libgmock.a

third_party_gtest_all = out/$(BUILD_MODE)/third_party/googletest/gtest-all.o
third_party_gmock_all = out/$(BUILD_MODE)/third_party/googletest/gmock-all.o

$(third_party_gtest_all) : third_party/googletest/googletest/src/gtest-all.cc
	mkdir -p $(dir $@)
	$(CXX) $(DEFS_$(BUILD_MODE)) $(INCLUDES) $(CFLAGS) $(CFLAGS_$(BUILD_MODE)) $(CFLAGS_CC) -c -o $@ $< \
		-I third_party/googletest/googletest/

$(third_party_gmock_all) : third_party/googletest/googlemock/src/gmock-all.cc
	mkdir -p $(dir $@)
	$(CXX) $(DEFS_$(BUILD_MODE)) $(INCLUDES) $(CFLAGS) $(CFLAGS_$(BUILD_MODE)) $(CFLAGS_CC) -c -o $@ $< \
		-I third_party/googletest/googlemock/

$(third_party_gtest_lib) : $(third_party_gtest_all)
	mkdir -p $(dir $@)
	$(AR) crs $@ $^

$(third_party_gmock_lib) : $(third_party_gmock_all)
	mkdir -p $(dir $@)
	$(AR) crs $@ $^

clean-gtest :
	rm -rf $(third_party_gtest_lib) $(third_party_gmock_lib) \
		$(third_party_gtest_all) $(third_party_gmock_all)

###
# libevhtp (third_party)

third_party_libevhtp = out/$(BUILD_MODE)/third_party/libevhtp
third_party_libevhtp_lib = $(third_party_libevhtp)/libevhtp.a
third_party_libevhtp_header = $(third_party_libevhtp)/evhtp-config.h

$(third_party_libevhtp_header) :
	mkdir -p $(dir $@)
	cd $(dir $@) && cmake -D EVHTP_DISABLE_REGEX:BOOL=ON $(PWD)/third_party/libevhtp

$(third_party_libevhtp_lib) : $(third_party_libevhtp_header)
	$(MAKE) -C $(third_party_libevhtp)

clean-libevhtp :
	rm -rf $(third_party_libevhtp)

# These settings are exported for other code to use as needed.
USE_INTERNAL_LIBEVHTP ?= 1

ifeq (1, $(USE_INTERNAL_LIBEVHTP))
LIBEVHTP_INCLUDES = -Ithird_party/libevhtp -I$(dir $(third_party_libevhtp_header))
LIBEVHTP_HEADERS = $(third_party_libevhtp_header)
else
LIBEVHTP_INCLUDES =
LIBEVHTP_HEADERS =
endif
