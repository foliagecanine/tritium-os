#!/bin/sh
set -e
. ./rpi-iso.sh

qemu-system-$(./target-triplet-to-arch.sh $HOST) -cdrom myos.iso
