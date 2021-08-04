#!/bin/sh
set -e
. ./scripts/iso.sh

qemu-system-$(./scripts/target-triplet-to-arch.sh $HOST) -m 512M -cdrom myos.iso -s -serial stdio -boot d -device ahci,id=ahci -drive file=$1,id=disk,if=none,format=raw -device ide-hd,drive=disk,bus=ahci.0 $2
