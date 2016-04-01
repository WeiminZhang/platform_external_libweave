# Copyright 2016 The Weave Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Logic to easily run cross-compiling tests.

CROSS_ROOT := $(PWD)/third_party/cross
CROSS_FLAGS :=
DOWNLOAD_CROSS_TOOLCHAINS := no
QEMU_BASE := $(CROSS_ROOT)/app-emulation/qemu/usr/bin

define cross-setup-gcc
.PHONY: $(1)
ifneq (,$$(findstring $(1),$$(MAKECMDGOALS)))
DOWNLOAD_CROSS_TOOLCHAINS := yes
CHOST := $(2)
BOARD := $(3)
CROSS := $$(CROSS_ROOT)/$$(CHOST)/bin/$$(CHOST)-
CC := $$(CROSS)gcc
CXX := $$(CROSS)g++
AR := $$(CROSS)ar
CROSS_FLAGS += $(5)
QEMU := $$(QEMU_BASE)/$(4) -L $$(CROSS_ROOT)/$$(BOARD)
endif
endef

# Whitespace matters with arguments, so we can't make this more readable :/.
$(eval $(call cross-setup-gcc,cross-arm,armv7a-cros-linux-gnueabi,arm-generic-full,qemu-arm,-mhard-float))
$(eval $(call cross-setup-gcc,cross-mipsel,mipsel-cros-linux-gnu,mipsel-o32-generic-full,qemu-mipsel))
$(eval $(call cross-setup-gcc,cross-x86,i686-pc-linux-gnu,x86-generic-full,qemu-i386))
$(eval $(call cross-setup-gcc,cross-x86_64,x86_64-cros-linux-gnu,amd64-generic-full,qemu-x86_64))

ifeq ($(DOWNLOAD_CROSS_TOOLCHAINS),yes)
ifeq (,$(wildcard third_party/cross/$(BOARD)))
CROSS_FETCH_OUT := $(shell ./third_party/get_cross.sh >&2)
endif
CROSS_FLAGS += --sysroot $(CROSS_ROOT)/$(BOARD)
CC += $(CROSS_FLAGS)
CXX += $(CROSS_FLAGS)
endif
