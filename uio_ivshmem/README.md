# IVSHMEM PCI/UIO

## Overview

Provides Linux PCI/UIO support for AxVisor IVSHMEM devices. The driver
exposes BAR0 registers and BAR2 shared memory through `/dev/uioN`, and
handles MSI-X notifications and interrupt re-enabling. Applications access
the mappings directly and implement their own communication protocols.

```text
uio_ivshmem/                         IVSHMEM feature in axvisor.ko
├── axvisor_ivshmem.h                driver registration and teardown interface
└── uio_ivshmem.c                    PCI driver (uio_ivshmem)
    └── Linux UIO core
        └── /dev/uioN                userspace access
            ├── map0: registers      BAR0
            ├── map1: shared         BAR2
            └── interrupt events     one MSI-X vector
```

## Usage

Build from the repository root; see [kernel requirements](../README.md#kernel-requirements).

```bash
make kernel_module KDIR=/PATH/TO/linux-build/ ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
```

After copying `axvisor.ko` to the target Linux guest:

```bash
insmod axvisor.ko

# Identify the matching UIO device and inspect its mappings
lspci -nn -d 1af4:1110
grep -H . /sys/class/uio/uio*/name
# Replace uio0 with the device whose name is uio_ivshmem
grep -H . /sys/class/uio/uio0/maps/map*/name \
          /sys/class/uio/uio0/maps/map*/addr \
          /sys/class/uio/uio0/maps/map*/size

# After applications release the device and mappings
rmmod axvisor
```
