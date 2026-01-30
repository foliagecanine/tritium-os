#ifndef _FS_FILE_H
#define _FS_FILE_H

#include <kernel/stdio.h>
#include <fs/fs.h>
#include <fs/disk.h>
#include <fs/fat12.h>
#include <fs/fat16.h>

FILE fopen(const char *filename,const char *mode);
uint8_t unmount_drive(uint32_t mount);
uint8_t mount_drive(uint32_t drive_num);
FSMOUNT get_disk_mount(uint32_t drive);
uint8_t fread(FILE *file, char *buf, uint64_t start, uint64_t len);
uint8_t fwrite(FILE *file, char *buf, uint64_t start, uint64_t len);
FILE readdir(FILE *file, char* buf, uint32_t n);
FILE fcreate(char *filename);
uint8_t fdelete(char *filename);
uint8_t fmove(char *src_filename, char *dest_filename);
void init_file();

#endif
