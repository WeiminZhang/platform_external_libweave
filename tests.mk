# Copyright 2015 The Weave Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

###
# tests

weave_test_obj_files := $(WEAVE_TEST_SRC_FILES:%.cc=out/$(BUILD_MODE)/%.o)

# We don't need libgtest.a, but the headers files in third_party/include.
$(weave_test_obj_files) : out/$(BUILD_MODE)/%.o : %.cc third_party/lib/libgtest.a
	mkdir -p $(dir $@)
	$(CXX) $(DEFS_$(BUILD_MODE)) $(INCLUDES) $(CFLAGS) $(CFLAGS_$(BUILD_MODE)) $(CFLAGS_CC) -c -o $@ $<

out/$(BUILD_MODE)/libweave-test.a : $(weave_test_obj_files)
	$(AR) crs $@ $^

weave_unittest_obj_files := $(WEAVE_UNITTEST_SRC_FILES:%.cc=out/$(BUILD_MODE)/%.o)

# We don't need libgtest.a, but the headers files in third_party/include.
$(weave_unittest_obj_files) : out/$(BUILD_MODE)/%.o : %.cc third_party/lib/libgtest.a
	mkdir -p $(dir $@)
	$(CXX) $(DEFS_$(BUILD_MODE)) $(INCLUDES) $(CFLAGS) $(CFLAGS_$(BUILD_MODE)) $(CFLAGS_CC) -c -o $@ $<

out/$(BUILD_MODE)/libweave_testrunner : $(weave_unittest_obj_files) $(third_party_chromium_crypto_unittest_obj_files) $(third_party_chromium_base_unittest_obj_files) out/$(BUILD_MODE)/libweave_common.a out/$(BUILD_MODE)/libweave-test.a
	$(CXX) -o $@ $^ $(CFLAGS) -lcrypto -lexpat -lgmock -lgtest -lpthread -lrt -Lthird_party/lib

test : out/$(BUILD_MODE)/libweave_testrunner
	$<

###
# export tests

weave_exports_unittest_obj_files := $(WEAVE_EXPORTS_UNITTEST_SRC_FILES:%.cc=out/$(BUILD_MODE)/%.o)

# We don't need libgtest.a, but the headers files in third_party/include.
$(weave_exports_unittest_obj_files) : out/$(BUILD_MODE)/%.o : %.cc third_party/lib/libgtest.a
	mkdir -p $(dir $@)
	$(CXX) $(DEFS_$(BUILD_MODE)) $(INCLUDES) $(CFLAGS) $(CFLAGS_$(BUILD_MODE)) $(CFLAGS_CC) -c -o $@ $<

out/$(BUILD_MODE)/libweave_exports_testrunner : $(weave_exports_unittest_obj_files) out/$(BUILD_MODE)/libweave.so out/$(BUILD_MODE)/libweave-test.a out/$(BUILD_MODE)/src/test/weave_testrunner.o
	$(CXX) -o $@ $^ $(CFLAGS) -lcrypto -lexpat -lgmock -lgtest -lpthread -lrt -Lthird_party/lib -Wl,-rpath=out/$(BUILD_MODE)/

export-test : out/$(BUILD_MODE)/libweave_exports_testrunner
	$<

.PHONY : test export-test

