#!/bin/bash
# Copyright 2015 The Weave Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Make gtest and gmock.
cd $(dirname "$0")
THIRD_PARTY=$(pwd)

mkdir -p include lib

rm -rf $THIRD_PARTY/googletest
git clone https://github.com/google/googletest.git || exit 1
cd googletest

# gtest is in process of changing of dir structure and it has broken build
# files. So this is temporarily workaround to fix that.
git reset --hard d945d8c000a0ade73585d143532266968339bbb3
mv googletest googlemock/gtest

for SUB_DIR in googlemock/gtest googlemock; do
  cd $THIRD_PARTY/googletest/$SUB_DIR || exit 1
  autoreconf -fvi || exit 1
  ./configure --disable-shared || exit 1
  make || exit 1
  cp -rf include/* $THIRD_PARTY/include/ || exit 1
  cp -rf lib/.libs/* $THIRD_PARTY/lib/ || exit 1
done

rm -rf $THIRD_PARTY/googletest
