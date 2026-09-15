# IVC

## Overview

Provides Linux support for AxVisor inter-VM communication: channel
creation and subscription, shared-memory message transfer, and message
fragmentation and reassembly. Applications manage channels through
`/dev/axivc` and exchange messages through publisher/subscriber devices.

```text
ivc/                         IVC feature in axvisor.ko
├── main.c                   unified module initialization and cleanup
├── ivc.c                    /dev/axivc* devices and ioctl/read/write
├── hvc.c                    hypervisor calls
├── region.c                 shared-memory region setup and validation
├── ring.c                   directional SPSC rings
├── message.c                message fragmentation and reassembly
├── includes/                internal driver headers
└── uapi/                    ABI shared with userspace
    ├── ioctl_args.h         ioctl commands and argument structures
    └── ivc_dev.h            device names
```

## Usage

Build from the repository root; see [kernel requirements](../README.md#kernel-requirements).

```bash
make kernel_module KDIR=/PATH/TO/linux-build/ ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
make demo CROSS=aarch64-linux-musl-
```

After copying `axvisor.ko` and `build/demo/` to the Linux guests under AxVisor:

```bash
# On each guest
insmod axvisor.ko

# Publisher guest (VM ID 2 in this example)
./demo/publish 0xdeadbeef

# Subscriber guest: connect while the publisher is running
./demo/subscribe 2 0xdeadbeef

# On each guest, after applications exit
rmmod axvisor
```
