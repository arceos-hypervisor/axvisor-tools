#!/bin/bash
set -x

JH_KO=./out/jailhouse.ko
JH=./out/jailhouse

if [ -z "$1" ]; then
    echo "Usage: $0 <parameter>"
    exit 1
fi

sudo $JH disable
sudo rmmod jailhouse
sudo insmod $JH_KO
sudo chown $(whoami) /dev/jailhouse
sudo $JH enable "$1"
