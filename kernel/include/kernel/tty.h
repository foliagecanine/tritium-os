#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H

#include <kernel/stdio.h>

void terminal_initialize(void);
void terminal_putchar(char c);
void terminal_write(const char* data, size_t size);
void terminal_writestring(const char* data);
void terminal_setcolor(uint8_t color);
uint8_t terminal_getcolor();

#endif
