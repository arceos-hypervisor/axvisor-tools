* compile
```bash
CROSS_COMPILE=aarch64-linux-gnu- ARCH=arm64 KDIR=/PATH/TO/linux-5.10.198/ make
```

* copy to guest
```bash
scp -P 5555 axvisor.ko root@localhost
```

* protocol

The channel devices use region v3 and Message V1. Each directional ring has
32 opaque 256-byte slots: a ring is 8448 bytes, the two rings start at offsets
256 and 8704, and the full region is 17152 bytes. These layout parameters are
part of the compatibility contract with the Rust `axivc` peer and must change
in lockstep even when the region version stays 3.

One `read(2)` or `write(2)` transfers one complete logical message. The POSIX
adapter rejects transport-level empty messages because a zero return value is
already used for an empty ring.

* test
```bash
scp -P 5555 -r ../ivc_demos root@localhost:
ssh -p 5555 root@localhost

# in guest
insmod axvisor.ko
cd ~/ivc_demos

gcc axivc_subscribe.c -o axivc_subscribe
./axivc_subscribe 2 0xdeadbeef
```


