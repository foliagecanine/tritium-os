#ifndef _FS_FAT_H
#define _FS_FAT_H

#include <kernel/stdio.h>

int findCharInArray(char * array, char c);
char intToChar(uint8_t num);
void LongToShortFilename(char * longfn, char * shortfn);

#endif