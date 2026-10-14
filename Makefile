# Top-level Makefile: builds lcb_protocol for the RK3566 board (aarch64 Buildroot)
# with the SDK inside the rk3566 build container.
#
#   docker start -ai rk3566_frankhung
#   cd /home/build/rk3566_dev/JHT_Ccode_LCB_machine_control_lib
#   make            # out/aarch64/lib/liblcb_protocol.so (+ .a)
#   make test       # build test programs and run them under qemu (see QEMU below)
#   make package    # out/aarch64/package/{lib,include/lcb_protocol}
#   make info       # show the .so's architecture and library dependencies
#   make clean
#
# Native (x86_64) build of the same sources:
#   make CROSS_COMPILE= ARCH_NAME=x86_64

SDK           ?= /opt/rk3566-dev/sdk
CROSS_COMPILE ?= $(SDK)/bin/aarch64-buildroot-linux-gnu-
SYSROOT       ?= $(SDK)/aarch64-buildroot-linux-gnu/sysroot
ARCH_NAME     ?= aarch64
# Empty: run aarch64 programs through the host's binfmt qemu (needs qemu 5+ with
# the F flag registered on the Docker host; the container's own qemu 2.11 cannot
# run Cortex-A55 code). Set e.g. QEMU=qemu-aarch64 to call an emulator directly.
QEMU          ?=

OUT    := $(CURDIR)/out/$(ARCH_NAME)
LCBDIR := $(CURDIR)/lcb_protocol

ifeq ($(CROSS_COMPILE),)
RUN :=
else ifeq ($(QEMU),)
RUN := env QEMU_LD_PREFIX=$(SYSROOT)
else
RUN := $(QEMU) -L $(SYSROOT)
endif

LCB_MAKE = $(MAKE) -C $(LCBDIR) \
           CC=$(CROSS_COMPILE)gcc AR=$(CROSS_COMPILE)ar \
           OBJDIR=$(OUT)/obj LIBDIR=$(OUT)/lib BINDIR=$(OUT)/bin

.PHONY: all test example package info clean help

all:
	+$(LCB_MAKE) all

test:
	+$(LCB_MAKE) test
	$(RUN) $(OUT)/bin/test_lcb
	$(RUN) $(OUT)/bin/test_constants

example:
	+$(LCB_MAKE) example

package:
	+$(LCB_MAKE) install PREFIX=$(OUT)/package

info:
	+@$(LCB_MAKE) --no-print-directory -s all
	@file $(OUT)/lib/liblcb_protocol.so
	@$(CROSS_COMPILE)readelf -d $(OUT)/lib/liblcb_protocol.so | grep NEEDED
	@echo "Required GLIBC symbol versions:"
	@$(CROSS_COMPILE)objdump -T $(OUT)/lib/liblcb_protocol.so | grep -o 'GLIBC_[0-9.]*' | sort -uV

clean:
	rm -rf $(OUT)

help:
	@echo "Targets: all (default), test, example, package, info, clean"
	@echo "Output:  $(OUT)"
	@echo "Variables: SDK CROSS_COMPILE SYSROOT ARCH_NAME QEMU"
