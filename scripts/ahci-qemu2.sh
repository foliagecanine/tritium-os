#!/bin/sh
set -e
. ./scripts/build.sh

qemu-system-$(./scripts/target-triplet-to-arch.sh $HOST) -m 512M -s -serial stdio -device ahci,id=ahci -drive file=$2,id=disk2,if=none,format=raw -drive file=$1,id=disk,if=none,format=raw -device ide-hd,drive=disk,bus=ahci.0 -device ide-hd,drive=disk2,bus=ahci.1 $3
