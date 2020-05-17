#ifndef _KERNEL_KSETUP_H
#define _KERNEL_KSETUP_H

#include <kernel/stdio.h>
#include <kernel/idt.h>
#include <kernel/pit.h>
#include <kernel/kbd.h>
#include <kernel/multiboot.h>

void init_gdt();
void init_paging(multiboot_info_t *mbi);
void identity_map(void *addr);
void map_addr(void *vaddr, void *paddr);
void unmap_vaddr(void *vaddr);
void* alloc_page(size_t pages);
void free_page(void *start, size_t pages);

#endif