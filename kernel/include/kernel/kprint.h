#ifndef _KERNEL_KPRINT_H
#define _KERNEL_KPRINT_H

#include <kernel/stdio.h>
#include <kernel/tty.h>

#define PANIC(x) terminal_setcolor(0x0c); printf("\nKERNEL PANIC: %s: %s:%d\n",x,__FILE__,(unsigned int)__LINE__); abort();

void kerror(const char* str);
void kprint(const char* str);

#endif