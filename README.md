# axvisor-tools

Tools and drivers for the AxVisor hypervisor.

## Layout

```text
Kbuild                 Kbuild rules: links both features into a single axvisor.ko
Makefile               top-level orchestration: libivc / demo / kernel_module
ivc/                   IVC kernel driver feature (sources only)
uio_ivshmem/           IVSHMEM PCI/UIO driver feature (sources only)
ivc/uapi/              ioctl ABI and device names shared by kernel and userspace
userspace/ivc/         IVC userspace library
demo/                  userspace demos
pybind/                Python bindings
axcli/                 CLI tool
```

## Dependencies

```text
demo/ + pybind/                                 IVC consumers
└── userspace/ivc/                              builds build/libivc.a / libivc.so
    ├── ulib.h                                  public user-space API
    └── /dev/axivc*                             runtime access via ioctl/read/write
        ├── ivc/uapi/{ioctl_args.h,ivc_dev.h}   shared driver ABI
        └── axvisor.ko                          kernel module (not linked with libivc)
            ├── ivc/                            IVC driver
            └── uio_ivshmem/                    PCI/UIO driver → /dev/uioN

demo/
└── app_proto.{h,c}                             demo-only; not part of libivc
```

User-space include paths: `-Iivc/uapi -Iuserspace/ivc`.

## Deliverable: one kernel module

All kernel driver features are compiled and linked into a single module,
`axvisor.ko`.

## Kernel requirements

The target kernel must be built with:

```text
CONFIG_MODULES=y
CONFIG_PCI=y
CONFIG_PCI_MSI=y
CONFIG_UIO=y
```

For the AArch64 QEMU MSI-X path, the interrupt controller must support ITS:

```text
CONFIG_ARM_GIC_V3_ITS=y
```

Set `KDIR` to the build directory of the kernel that will load `axvisor.ko`.

## Build

```bash
# kernel module -> ./axvisor.ko
make kernel_module KDIR=/PATH/TO/linux-build/ ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-

# userspace library and demos -> build/
make
```

## Load / unload

```bash
insmod axvisor.ko    # registers IVC devices and the IVSHMEM PCI/UIO driver
rmmod axvisor        # unregisters both, in reverse order
```

See [IVC](ivc/README.md) and [IVSHMEM](uio_ivshmem/README.md) for feature details.
