#ifndef _KERNEL_KPRINT_H
#define _KERNEL_KPRINT_H

#include <kernel/stdio.h>
#include <kernel/tty.h>

void panic(const char* str);
void kerror(const char* str);
void kprint(const char* str);

#endif