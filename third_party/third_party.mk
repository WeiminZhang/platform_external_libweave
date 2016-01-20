# Copyright 2015 The Weave Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

###
# third_party/chromium/

third_party_chromium_base_obj_files := $(THIRD_PARTY_CHROMIUM_BASE_SRC_FILES:%.cc=out/$(BUILD_MODE)/%.o)

$(third_party_chromium_base_obj_files) : out/$(BUILD_MODE)/%.o : %.cc third_party/include/gtest/gtest.h
	mkdir -p $(dir $@)
	$(CXX) $(DEFS_$(BUILD_MODE)) $(INCLUDES) $(CFLAGS) $(CFLAGS_$(BUILD_MODE)) $(CFLAGS_CC) -c -o $@ $<

third_party_chromium_base_unittest_obj_files := $(THIRD_PARTY_CHROMIUM_BASE_UNITTEST_SRC_FILES:%.cc=out/$(BUILD_MODE)/%.o)

$(third_party_chromium_base_unittest_obj_files) : out/$(BUILD_MODE)/%.o : %.cc third_party/include/gtest/gtest.h
	mkdir -p $(dir $@)
	$(CXX) $(DEFS_$(BUILD_MODE)) $(INCLUDES) $(CFLAGS) $(CFLAGS_$(BUILD_MODE)) $(CFLAGS_CC) -c -o $@ $<

third_party_chromium_crypto_obj_files := $(THIRD_PARTY_CHROMIUM_CRYPTO_SRC_FILES:%.cc=out/$(BUILD_MODE)/%.o)

$(third_party_chromium_crypto_obj_files) : out/$(BUILD_MODE)/%.o : %.cc third_party/include/gtest/gtest.h
	mkdir -p $(dir $@)
	$(CXX) $(DEFS_$(BUILD_MODE)) $(INCLUDES) $(CFLAGS) $(CFLAGS_$(BUILD_MODE)) $(CFLAGS_CC) -c -o $@ $<

third_party_chromium_crypto_unittest_obj_files := $(THIRD_PARTY_CHROMIUM_CRYPTO_UNITTEST_SRC_FILES:%.cc=out/$(BUILD_MODE)/%.o)

$(third_party_chromium_crypto_unittest_obj_files) : out/$(BUILD_MODE)/%.o : %.cc third_party/include/gtest/gtest.h
	mkdir -p $(dir $@)
	$(CXX) $(DEFS_$(BUILD_MODE)) $(INCLUDES) $(CFLAGS) $(CFLAGS_$(BUILD_MODE)) $(CFLAGS_CC) -c -o $@ $<

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
# libgtest and libgmock (third_party, downloaded on build)

third_party/include/gtest/gtest.h :
	@echo Downloading and building libgtest and libgmock...
	third_party/get_gtest.sh
	@echo Finished downloading and building libgtest and libgmock.

clean-gtest :
	rm -rf third_party/include/gtest third_party/include/gmock
	rm -rf third_party/lib/libgmock* third_party/lib/libgtest*
	rm -rf third_party/googletest

###
# libevent (third_party, downloaded on build)

third_party/include/event2/event.h :
	@echo Downloading and building libevent...
	DISABLE_LIBEVENT_TEST=1 third_party/get_libevent.sh
	@echo Finished downloading and building libevent.

clean-libevent :
	rm -rf third_party/include/ev* third_party/include/event2
	rm -rf third_party/lib/libevent*
	rm -rf third_party/libevent

