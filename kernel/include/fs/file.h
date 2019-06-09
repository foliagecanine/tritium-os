#ifndef _FS_FILE_H
#define _FS_FILE_H

#include <kernel/stdio.h>
#include <fs/fs.h>
#include <fs/disk.h>
#include <fs/fat12.h>

FILE fopen(const char *filename,const char *mode);
uint8_t unmountDrive(uint8_t drive);
uint8_t mountDrive(uint8_t drive);

#endif