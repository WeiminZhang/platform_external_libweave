#!/bin/sh
#
# Produces a source code snapshot. Generates the changelog and deals with
# external dependencies. Run this from one directory above the sources.
#
# FIXME: specify new-tag and prev-tag as cmdline args
#

PREV_TAG=
NEW_TAG=0.0.1

saved=$PWD

cd libweave
git clean -f

# create changelog
if [ -n "$PREV_TAG" ]; then
  git log --summary --format=short ${PREV_TAG}.. >ChangeLog
else
  git log --summary --format=short >ChangeLog
fi

cd third_party
rm -rf googletest
# TODO(proppy): investigate using libgtest-dev and google-mock
git clone https://github.com/google/googletest.git || exit 1
cd googletest
git reset --hard d945d8c000a0ade73585d143532266968339bbb3
mv googletest googlemock/gtest
cd $saved
tar --exclude-vcs --transform="s/^libweave/libweave-${NEW_TAG}/" --exclude="out" --exclude="debian" -czf libweave-${NEW_TAG}.tar.gz libweave

