#!/bin/bash
# Copyright 2015 The Weave Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Make libevent.
# Example uses libevent to implement HTTPS server. This capability is
# available only in version 2.1.x-alpha. Step could be replaced with apt-get
# in future.
cd $(dirname "$0")
THIRD_PARTY=$(pwd)

mkdir -p include lib

rm -rf $THIRD_PARTY/libevent
git clone https://github.com/libevent/libevent.git || exit 1
cd libevent || exit 1

./autogen.sh || exit 1
./configure --disable-shared || exit 1
make || exit 1
if [ -z "$DISABLE_LIBEVENT_TEST" ]; then
  echo -e "\n\nTesting libevent...\nCan take several minutes.\n"
  make verify || exit 1
fi
cp -rf include/*.h include/event2 $THIRD_PARTY/include/ || exit 1
cp -f .libs/lib* $THIRD_PARTY/lib/ || exit 1

rm -rf $THIRD_PARTY/libevent
