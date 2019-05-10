#!/bin/sh
set -e
. ./iso.sh

qemu-system-$(./target-triplet-to-arch.sh $HOST) -m 512M -cdrom myos.iso -s -serial stdio $1 $2
