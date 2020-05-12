#!/bin/sh
set -e
. ./iso.sh

qemu-system-$(./target-triplet-to-arch.sh $HOST) -m 512M -cdrom myos.iso -s -serial stdio -boot d -device ahci,id=ahci -drive file=$1,id=disk,if=none -device ide-drive,drive=disk,bus=ahci.0 $2
