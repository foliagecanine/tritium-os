#ifndef _KERNEL_MMU_H
#define _KERNEL_MMU_H

#include <kernel/stdio.h>

void mmu_init(uint32_t krnend);
char *strcpy(char *dest, const char *src);
void* malloc(size_t size);
void free(void *__ptr);

#endif