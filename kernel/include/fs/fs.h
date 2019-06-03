#ifndef _KERNEL_FS_H
#define _KERNEL_FS_H

#include <kernel/stdio.h>
#include <kernel/easylib.h>

typedef struct {
	uint64_t location;
	uint8_t device;
	_Bool writelock;
} FILE, *PFILE;

typedef struct {
	char type[6];
	uint8_t drive;
	void *mount;
	_Bool mountEnabled;
} FSMOUNT;

#endif