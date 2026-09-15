# Build all kernel driver features into axvisor.ko.
# Invoked by the top-level Makefile through:
#   make -C <kernel build tree> M=<repository root> modules

EXTRA_CFLAGS ?=
ccflags-y := $(EXTRA_CFLAGS) -I$(src)/ivc/uapi -I$(src)/uio_ivshmem

obj-m := axvisor.o

# IVC feature (ivc/)
axvisor-objs := ivc/main.o \
	ivc/hvc.o \
	ivc/ring.o \
	ivc/message.o \
	ivc/region.o \
	ivc/ivc.o

# IVSHMEM PCI/UIO feature (uio_ivshmem/)
axvisor-objs += uio_ivshmem/uio_ivshmem.o
