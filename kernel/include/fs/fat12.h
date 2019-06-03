#ifndef _FS_FAT12_H
#define _FS_FAT12_H

#include <kernel/stdio.h>
#include <kernel/easylib.h>
#include <fs/disk.h>
#include <fs/fs.h>

FSMOUNT MountFAT12(uint8_t drive_num);

#endif