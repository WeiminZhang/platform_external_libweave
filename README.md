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

Download the Repo tool and ensure that it is executable:

```
curl https://storage.googleapis.com/git-repo-downloads/repo > ~/bin/repo
chmod a+x ~/bin/repo
```

# Checkout code

```
repo init -u https://weave.googlesource.com/weave/manifest
repo sync
```

# Directory structure

| Path           | Description                        |
|----------------|------------------------------------|
| include/       | Includes to be used by device code |
| src/           | Implementation sources             |
| examples/      | Example of device code             |
| third_party/   | Dependencies                       |
| \*.gyp\* files | Build files                        |


# Quick start on Debian/Ubuntu

### Install prerequisites

```
examples/prerequisites.sh
```

### Build library, tests, run tests, build example

```
examples/build.sh
```

### Execute example (see this [README](examples/daemon/README.md) for details):

```
sudo out/Debug/weave_daemon
```

# Prerequisites

### Common

  - autoconf
  - automake
  - binutils
  - libtool
  - gyp
  - libexpat1-dev

### For tests

  - gtest
  - gmock

### For examples

  - hostapd
  - libavahi-client-dev
  - libcurl4-openssl-dev
  - libevent 2.1.x-alpha


# Compiling

### Generate build files

```
gyp -I libweave_common.gypi --toplevel-dir=. --depth=. \
    -f make libweave_standalone.gyp
```

### Build library with tests

```
make
```

### Build library only

```
make libweave
```

# Testing

### Run unittests tests

```
out/Debug/libweave_testrunner
out/Debug/libweave_exports_testrunner
```

# Making changes

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

Go to the url from the output of "repo upload" and add reviewers.
