#!/bin/bash
set -e

# Create a disk image template with GRUB installed
# Usage: ./create_disk_template.sh [filename] [size_in_MB]

SIZE_MB=${2:-32}  # Default 32MiB
IMAGE_NAME=${1:-fs/disk_template.img}

echo "Creating $SIZE_MB MiB disk image: $IMAGE_NAME"

# Create empty image
dd if=/dev/zero of="$IMAGE_NAME" bs=1M count="$SIZE_MB"

# Set up loop device
LOOP_DEV=$(sudo losetup -f)
sudo losetup "$LOOP_DEV" "$IMAGE_NAME"

# Format the whole disk as FAT16 (no partition table)
sudo mkfs.vfat -F 16 "$LOOP_DEV"

# Mount the disk
MOUNT_DIR="/tmp/grub_template"
sudo mkdir -p "$MOUNT_DIR"
sudo mount "$LOOP_DEV" "$MOUNT_DIR"

# Create GRUB directory structure
sudo mkdir -p "$MOUNT_DIR/BOOT/GRUB"

# Create a basic grub.cfg
sudo tee "$MOUNT_DIR/BOOT/GRUB/grub.cfg" > /dev/null << EOF
menuentry "myos" {
	multiboot /BOOT/myos.kernel
	set gfxmode=text
}
EOF

# Install GRUB on the loop device
sudo grub-install --force --target=i386-pc --boot-directory="$MOUNT_DIR/BOOT" "$LOOP_DEV"

# Unmount and clean up
sync
sudo umount "$MOUNT_DIR"
sudo losetup -d "$LOOP_DEV"
sudo rmdir "$MOUNT_DIR"

# Mark BOOT folder as hidden
mattrib -i "$IMAGE_NAME" +h ::BOOT

echo "Template disk image created: $IMAGE_NAME"
