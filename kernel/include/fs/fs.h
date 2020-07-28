#ifndef _KERNEL_FS_H
#define _KERNEL_FS_H

#include <kernel/stdio.h>

#define SUCCESS 			0
#define FILE_NOT_FOUND		1
#define IS_DIRECTORY 		2
#define INVALID_FNAME		3
#define UNKNOWN_ERROR		4
#define INCORRECT_FS_TYPE 	5

typedef struct {
	uint64_t location;
	uint64_t size;
	uint8_t mountNumber;
	uint32_t clusterNumber; //For FAT based systems
	uint64_t dir_entry;
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