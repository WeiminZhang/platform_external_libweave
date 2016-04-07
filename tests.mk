# Copyright 2015 The Weave Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

###
# tests

TEST_FLAGS ?= \
	--gtest_break_on_failure

TEST_ENV ?=
ifeq (1, $(CLANG))
  TEST_ENV += ASAN_SYMBOLIZER_PATH=$(shell which llvm-symbolizer-3.6)
endif
TEST_ENV += $(QEMU)

weave_test_obj_files := $(WEAVE_TEST_SRC_FILES:%.cc=out/$(BUILD_MODE)/%.o)

$(weave_test_obj_files) : out/$(BUILD_MODE)/%.o : %.cc
	mkdir -p $(dir $@)
	$(CXX) $(DEFS_$(BUILD_MODE)) $(INCLUDES) $(CFLAGS) $(CFLAGS_$(BUILD_MODE)) $(CFLAGS_CC) -c -o $@ $<

out/$(BUILD_MODE)/libweave-test.a : $(weave_test_obj_files)
	$(AR) crs $@ $^

weave_unittest_obj_files := $(WEAVE_UNITTEST_SRC_FILES:%.cc=out/$(BUILD_MODE)/%.o)

$(weave_unittest_obj_files) : out/$(BUILD_MODE)/%.o : %.cc
	mkdir -p $(dir $@)
	$(CXX) $(DEFS_TEST) $(INCLUDES) $(CFLAGS) $(CFLAGS_$(BUILD_MODE)) $(CFLAGS_CC) -c -o $@ $<

out/$(BUILD_MODE)/libweave_testrunner : \
	$(weave_unittest_obj_files) \
	$(third_party_chromium_crypto_unittest_obj_files) \
	$(third_party_chromium_base_unittest_obj_files) \
	out/$(BUILD_MODE)/libweave_common.a \
	out/$(BUILD_MODE)/libweave-test.a \
	$(third_party_gtest_lib) \
	$(third_party_gmock_lib)
	$(CXX) -o $@ $^ $(CFLAGS) -lcrypto -lexpat -lpthread -lrt

test : out/$(BUILD_MODE)/libweave_testrunner
	$(TEST_ENV) $< $(TEST_FLAGS)

###
# export tests

weave_exports_unittest_obj_files := $(WEAVE_EXPORTS_UNITTEST_SRC_FILES:%.cc=out/$(BUILD_MODE)/%.o)

$(weave_exports_unittest_obj_files) : out/$(BUILD_MODE)/%.o : %.cc
	mkdir -p $(dir $@)
	$(CXX) $(DEFS_TEST) $(INCLUDES) $(CFLAGS) $(CFLAGS_$(BUILD_MODE)) $(CFLAGS_CC) -c -o $@ $<

out/$(BUILD_MODE)/libweave_exports_testrunner : \
	$(weave_exports_unittest_obj_files) \
	out/$(BUILD_MODE)/libweave.so \
	out/$(BUILD_MODE)/libweave-test.a \
	out/$(BUILD_MODE)/src/test/weave_testrunner.o \
	$(third_party_gtest_lib) \
	$(third_party_gmock_lib)
	$(CXX) -o $@ $^ $(CFLAGS) -lcrypto -lexpat -lpthread -lrt -Wl,-rpath=out/$(BUILD_MODE)/

export-test : out/$(BUILD_MODE)/libweave_exports_testrunner
	$(TEST_ENV) $< $(TEST_FLAGS)

testall : test export-test
check : testall

###
# coverage
# This runs coverage against unit tests, invoke with "make coverage".
# Output "homepage" is out/$(BUILD_MODE)/coverage_html/index.html
# Running a mode other than Debug will result in incorrect coverage data.
# https://gcc.gnu.org/onlinedocs/gcc/Gcov-and-Optimization.html

coverage: CFLAGS+=--coverage

run_coverage: test
	lcov --capture --directory out/$(BUILD_MODE) --output-file out/$(BUILD_MODE)/coverage.info
	lcov -b . --remove out/$(BUILD_MODE)/coverage.info "*third_party*" "/usr/include/*" "*/include/weave/test/*" "*/src/test/*" "*/include/weave/provider/test/*" -o out/$(BUILD_MODE)/coverage_filtered.info
	genhtml out/$(BUILD_MODE)/coverage_filtered.info --output-directory out/$(BUILD_MODE)/coverage_html

coverage: run_coverage

.PHONY : check coverage run_coverage test export-test testall
