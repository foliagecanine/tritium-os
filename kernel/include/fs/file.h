#ifndef _FS_FILE_H
#define _FS_FILE_H

#include <kernel/stdio.h>
#include <fs/fs.h>
#include <fs/disk.h>
#include <fs/fat12.h>
#include <fs/fat16.h>

FILE fopen(const char *filename,const char *mode);
uint8_t unmountDrive(uint8_t drive);
uint8_t mountDrive(uint8_t drive);
FSMOUNT getDiskMount(uint8_t drive);
uint8_t fread(FILE *file, char *buf, uint64_t start, uint64_t len);
uint8_t fwrite(FILE *file, char *buf, uint64_t start, uint64_t len);
FILE readdir(FILE *file, char* buf, uint32_t n);
FILE fcreate(char *filename);
uint8_t fdelete(char *filename);
void init_file();

#endif
