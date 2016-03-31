# Overview

libWeave is the library with device side implementation of Weave protocol.

# Sources

Sources are located in git repository at
https://weave.googlesource.com/weave/libweave/


# Install Repo

Make sure you have a bin/ directory in your home directory
and that it is included in your path:

```
mkdir ~/bin
PATH=~/bin:$PATH
```

Download the [Repo tool](https://gerrit.googlesource.com/git-repo) and ensure
that it is executable:

```
curl https://storage.googleapis.com/git-repo-downloads/repo > ~/bin/repo
chmod a+x ~/bin/repo
```

# Checkout code

```
mkdir ~/weave
cd ~/weave
repo init -u https://weave.googlesource.com/weave/manifest
repo sync
```

This checks out libweave and its dependencies into the ~/weave directory.

# libweave Directory structure

| Path                     | Description                        |
|--------------------------|------------------------------------|
| include/                 | Includes to be used by device code |
| src/                     | Implementation sources             |
| examples/                | Example of device code             |
| third_party/             | Dependencies                       |
| Makefile, \*.mk files    | Build files                        |


# Quick start on Debian/Ubuntu

### Install prerequisites

```
sudo apt-get update
sudo apt-get install \
  autoconf \
  automake \
  binutils \
  g++ \
  hostapd \
  libavahi-client-dev \
  libcurl4-openssl-dev \
  libevent-dev \
  libexpat1-dev \
  libnl-3-dev \
  libnl-route-3-dev \
  libssl-dev \
  libtool
```

# Prerequisites

### Common

  - autoconf
  - automake
  - binutils
  - libtool
  - libexpat1-dev

### For tests

  - cmake
  - gtest (included; see third_party/googletest/googletest/)
  - gmock (included; see third_party/googletest/googlemock/)

### For examples

  - cmake
  - hostapd
  - libavahi-client-dev
  - libcurl4-openssl-dev
  - libevhtp (included; see third_party/libevhtp/)
  - libevent-dev


# Compiling

From the `libweave` directory:

The `make --jobs/-j` flag is encouraged, to speed up build time. For example

```
make -j
```

which happens to be the same as

```
make all -j
```

### Build library

```
make out/Debug/libweave.so
```

### Build examples

```
make all-examples
```

See [the examples README](/examples/daemon/README.md) for details.

### Cross-compiling

The build supports transparently downloading & using a few cross-compilers.
Just add `cross-<arch>` to the command line in addition to the target you
want to actually build.

This will cross-compile for an armv7 (hard float) target:

```
make cross-arm all-libs
```

This will cross-compile for a mips (little endian) target:

```
make cross-mipsel all-libs
```

# Testing

### Run tests

```
make test
make export-test
```

or

```
make testall
```

### Cross-testing

The build supports using qemu to run non-native tests.

This will run armv7 tests through qemu:

```
make cross-arm testall
```

# Making changes

The [Android Developing site](https://source.android.com/source/developing.html)
has a lot of good tips for working with git and repo in general.  The tips below
are meant as a quick cheat sheet rather than diving deep into relevant topics.

### Configure git
Make sure to have correct user in local or global config e.g.:

```
git config --local user.name "User Name"
git config --local user.email user.name@example.com
```

### Start local branch

```
repo start <branch name> .
```

### Edit code and commit locally e.g.

```
git commit -a -v
```

### Upload CL

```
repo upload .
```

### Request code review

Go to the url from the output of `repo upload` and add reviewers.
