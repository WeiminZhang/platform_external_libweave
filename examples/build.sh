#!/bin/bash
# Copyright 2015 The Weave Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

DIR=$(cd -P -- "$(dirname -- "$0")" && pwd -P)
ROOT_DIR=$(cd -P -- "$(dirname -- "$0")/.." && pwd -P)

cd $ROOT_DIR

gyp -Ilibweave_common.gypi --toplevel-dir=. --depth=. -f make $DIR/daemon/examples.gyp

if [ -z "$BUILD_CONFIG" ]; then
   export BUILD_CONFIG=Debug
fi

export BUILD_TARGET=$*
if [ -z "$BUILD_TARGET" ]; then
   export BUILD_TARGET="weave_daemon_examples libweave_testrunner libweave_exports_testrunner"
fi

export CORES=`cat /proc/cpuinfo | grep processor | wc -l`
BUILDTYPE=$BUILD_CONFIG make -j $CORES $BUILD_TARGET || exit 1

if [[ $BUILD_TARGET == *"libweave_testrunner"* ]]; then
  out/${BUILD_CONFIG}/libweave_testrunner --gtest_break_on_failure || exit 1
  out/${BUILD_CONFIG}/libweave_exports_testrunner --gtest_break_on_failure || exit 1
fi
