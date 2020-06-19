#ifndef _KERNEL_FS_H
#define _KERNEL_FS_H

#include <kernel/stdio.h>

typedef struct {
	uint64_t location;
	uint64_t size;
	uint8_t mountNumber;
	uint32_t clusterNumber; //For FAT based systems
	_Bool writelock;
	_Bool valid;
	_Bool directory;
} FILE, *PFILE;

typedef struct {
	char type[6];
	uint8_t drive;
	void *mount;
	_Bool mountEnabled;
} FSMOUNT;

#endif