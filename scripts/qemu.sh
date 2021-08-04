#!/bin/sh
set -e
. ./scripts/iso.sh

qemu-system-$(./scripts/target-triplet-to-arch.sh $HOST) -m 512M -cdrom myos.iso -s -serial stdio $1 $2
