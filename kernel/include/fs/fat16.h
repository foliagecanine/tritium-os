#ifndef _FS_FAT16_H
#define _FS_FAT16_H

#include <fs/disk.h>
#include <fs/fat.h>
#include <fs/file.h>
#include <fs/fs.h>
#include <kernel/stdio.h>

typedef struct
{
    uint32_t MntSig;
    uint32_t NumTotalSectors;
    uint32_t FATOffset;
    uint32_t NumRootDirectoryEntries;
    uint32_t FATEntrySize;
    uint32_t RootDirectoryOffset;
    uint32_t RootDirectorySize;
    uint32_t FATSize;
    uint32_t SectorsPerCluster;
    uint32_t SystemAreaSize;
} FAT16_MOUNT;

_Bool   detect_fat16(uint32_t drive_num);
FSMOUNT MountFAT16(uint32_t drive_num);
FILE    FAT16_fopen(uint32_t prevLocation, uint32_t numEntries, char *filename, uint32_t drive_num, FAT16_MOUNT fm,
                    uint8_t mode);
void    FAT16_print_folder(uint32_t location, uint32_t numEntries, uint32_t drive_num);
uint8_t FAT16_fread(FILE *file, char *buf, uint32_t start, uint32_t len, uint32_t drive_num);
uint8_t FAT16_fwrite(FILE *file, char *buf, uint32_t start, uint32_t len, uint32_t drive_num);
FILE    FAT16_readdir(FILE *file, char *buf, uint32_t n, uint32_t drive_num);
FILE    FAT16_fcreate(char *name, FAT16_MOUNT fm, uint32_t drive_num);
uint8_t FAT16_fdelete(char *name, uint32_t drive_num);
uint8_t FAT16_ferase(char *name, FAT16_MOUNT fm, uint32_t drive_num);
void    print_fat16_values(uint32_t drive_num);

#endif
