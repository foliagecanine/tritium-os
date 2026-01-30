#ifndef _KERNEL_KPRINT_H
#define _KERNEL_KPRINT_H

#include <kernel/stdio.h>
#include <kernel/tty.h>

#define PANIC(x) kpanic(x, __FILE__, __LINE__);

void kpanic(const char* str, const char* file, int line);
void kerror(const char* str);
void kprint(const char* str);
void kwarn(const char* str);
int kprintf(const char* format, ...);
const char* get_symbol_name(uint32_t addr);
void dump_stacktrace();

#endif