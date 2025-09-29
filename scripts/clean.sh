#!/bin/sh
set -e
. ./scripts/config.sh

rm -rf sysroot
rm -rf isodir

for PROJECT in $PROJECTS; do
  (cd $PROJECT && $MAKE clean)
done
#rm -rf myos.iso #we want to leave a usable iso.
