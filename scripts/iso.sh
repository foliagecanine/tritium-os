#!/bin/sh
set -e
. ./scripts/build.sh

mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

cp sysroot/boot/myos.kernel isodir/boot/myos.kernel
cat > isodir/boot/grub/grub.cfg << EOF
menuentry "myos" {
	multiboot /boot/myos.kernel
	set gfxmode=text
}
EOF
grub-mkrescue -o myos.iso isodir
