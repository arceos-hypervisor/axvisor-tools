# Makefile for building the UIO driver for the AxVisor ivshmem PCI profile.
#
# Build against a prepared kernel build tree:
#   make -C uio_ivshmem KDIR=<kernel build tree> \
#        KBUILD_EXTRA_SYMBOLS=<kernel build tree>/drivers/uio/Module.symvers
#
# The UIO core is built by the kernel itself. When UIO is a module, pass the
# Module.symvers generated beside it through KBUILD_EXTRA_SYMBOLS. Leave it
# empty when the kernel's own symbol table already exports UIO; importing the
# same symbols twice is an error.

ARCH ?= arm64
CROSS_COMPILE ?= aarch64-linux-gnu-
KDIR ?= /lib/modules/$(shell uname -r)/build
KBUILD_EXTRA_SYMBOLS ?=

CURRENT_PATH := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

ifneq ($(KBUILD_EXTRA_SYMBOLS),)
KBUILD_EXTRA_SYMBOLS_ARG := KBUILD_EXTRA_SYMBOLS=$(KBUILD_EXTRA_SYMBOLS)
endif

all:
	CROSS_COMPILE=$(CROSS_COMPILE) ARCH=$(ARCH) $(MAKE) -C $(KDIR) M=$(CURRENT_PATH) $(KBUILD_EXTRA_SYMBOLS_ARG) modules

clean:
	CROSS_COMPILE=$(CROSS_COMPILE) ARCH=$(ARCH) $(MAKE) -C $(KDIR) M=$(CURRENT_PATH) clean

obj-m += uio_ivshmem.o
