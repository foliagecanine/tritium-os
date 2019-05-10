#ifndef _KERNEL_FS_H
#define _KERNEL_FS_H

#include <kernel/stdio.h>

typedef struct {
	uint64_t location;
	uint8_t device;
	_Bool writelock;
} FILE, *PFILE;

#endif