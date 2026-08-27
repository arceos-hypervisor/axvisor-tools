# Axvisor IVC Linux Support

This directory is the source-of-truth for the Linux side of Axvisor IVC:

- `kernel_driver/`: the Linux kernel module that exposes `/dev/axivc` and the
  per-channel devices.
- `include/` and `ulib/`: the C userspace API and Message V1 application codec.
- `demo/`: the full-duplex Message V1 publisher and subscriber.
- `tests/`: host-side protocol, I/O, rollback, teardown, and lifetime
  regressions.
- `pybind/`: Python bindings over the same userspace API.

Build products are intentionally untracked. Image builders must rebuild the
module and both demos from a pinned commit rather than reuse an ELF from this
repository.

## Message V1 demo protocol

Each device `read()` or `write()` transfers one complete, non-empty Message V1
logical message. POSIX zero-length reads cannot distinguish an empty message
from an empty ring, so the Linux read/write adapter rejects empty messages even
though the transport codec can represent them. The kernel module handles cell
fragmentation and reassembly; the demos use this application payload:

```text
kind: u8 | sequence: u64 little-endian | body_len: u16 little-endian | body
```

The full-duplex demo sends five ordered Request messages with total lengths
`39, 40, 41, 640, 700`, three independently sequenced Data messages with total
lengths `41, 641, 700`, and one Ack for each Request. These lengths cover a
single cell, the fragment boundary, and messages larger than the ring's
in-flight capacity. Every receiver checks the message kind, application
sequence, exact length, and deterministic body bytes.

Region v3/Message V1 rejects the older fixed-slot region v2 layout. The Linux
demos and kernel module must therefore be built and deployed from matching
sources.

## Userspace API and lifetime

The publisher and subscriber APIs are symmetric:

```c
ivc_publisher_send(...);
ivc_publisher_recv(...);
ivc_subscriber_send(...);
ivc_subscriber_recv(...);
```

One successful send is one complete logical message. A short device write is
reported as an error and is never retried as a second message.

A publisher or subscriber keeps its manager busy until `ivc_unpublish()` or
`ivc_unsubscribe()` consumes the endpoint. `ivc_close_manager()` returns
`EBUSY` without consuming the manager while an endpoint remains. Endpoint
teardown attempts every cleanup step and preserves the first diagnostic
`errno`. Once no endpoint remains, closing the manager always consumes its
allocation, even if the underlying `close()` reports a late error.

## Build and test

Run all host-side regressions with the native compiler:

```bash
make -C ivc test
```

Build static AArch64 demos with a musl cross toolchain:

```bash
make -C ivc clean
make -C ivc demo ARCH=arm64 CROSS=aarch64-linux-musl-
```

The resulting executables are `ivc/build/demo/publish` and
`ivc/build/demo/subscribe`. Their command lines are:

```text
publish <channel_key> [channel_size]
subscribe <publisher_vm_id> <channel_key> [request_count]
```

The Message V1 demo requires `request_count` to be five.

Build the module against the exact prepared tree for the guest kernel:

```bash
make -C ivc kernel-module \
  ARCH=arm64 \
  CROSS_COMPILE=aarch64-unknown-linux-musl- \
  KDIR=/path/to/prepared/linux
```

The prepared tree must contain the guest kernel's `.config`, generated headers,
`Module.symvers`, `scripts/module.lds`, and built `scripts/mod/modpost`. Verify
that `modinfo -F vermagic ivc/kernel_driver/axvisor.ko` starts with the guest's
exact kernel release before packaging it into an image.
