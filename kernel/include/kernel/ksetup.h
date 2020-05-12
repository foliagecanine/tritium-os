#ifndef _KERNEL_KSETUP_H
#define _KERNEL_KSETUP_H

#include <kernel/stdio.h>
#include <kernel/idt.h>
#include <kernel/pit.h>
#include <kernel/kbd.h>

void init_gdt();
void init_paging();
void identity_map(void *addr);

#endif