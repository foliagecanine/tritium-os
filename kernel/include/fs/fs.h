#ifndef _KERNEL_FS_H
#define _KERNEL_FS_H

#include <kernel/stdio.h>
#include <kernel/easylib.h>

typedef struct {
	uint64_t location;
	uint64_t size;
	uint8_t mountNumber;
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