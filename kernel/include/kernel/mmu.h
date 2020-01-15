#ifndef _KERNEL_MMU_H
#define _KERNEL_MMU_H

#include <kernel/stdio.h>
#include <kernel/multiboot.h>

void mmu_info();
void mmu_init(uint32_t krnend);
char *strcpy(char *dest, const char *src);
void* malloc(size_t size);
void free(void *__ptr);
void init_mmu_paging();

void *alloc_physical_block();
void free_physical_block(void *ptr);

#endif