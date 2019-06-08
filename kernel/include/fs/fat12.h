#ifndef _FS_FAT12_H
#define _FS_FAT12_H

#include <kernel/stdio.h>
#include <kernel/easylib.h>
#include <fs/disk.h>
#include <fs/fs.h>

_Bool detect_fat12(uint8_t drive_num);
FSMOUNT MountFAT12(uint8_t drive_num);
FILE FAT12_fopen(uint32_t prevLocation, uint32_t numEntries, char *filename, uint8_t drive_num, uint8_t mode);

typedef struct {
	uint32_t MntSig;
	uint32_t NumTotalSectors;
	uint32_t FATOffset;
	uint32_t NumRootDirectoryEntries;
	uint32_t FATEntrySize;
	uint32_t RootDirectoryOffset;
	uint32_t RootDirectorySize;
	uint32_t FATSize;
} FAT12_MOUNT;

#endif