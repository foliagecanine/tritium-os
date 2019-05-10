#!/bin/sh
set -e
. ./rpi-iso.sh

qemu-system-$(./target-triplet-to-arch.sh $HOST) -m 512M -cdrom myos.iso -s -serial stdio -hda test.img
