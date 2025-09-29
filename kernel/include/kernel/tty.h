#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H

#include <kernel/stdio.h>

void terminal_initialize(void);
void terminal_putchar(char c);
void terminal_write(const char* data, size_t size);
void terminal_writestring(const char* data);
void terminal_setcolor(uint8_t color);
uint8_t terminal_getcolor();
void set_scroll(_Bool allow_scroll);
void disable_cursor();
void cursor_position(size_t x, size_t y);
void enable_cursor(uint8_t start, uint8_t end);
void terminal_backup();
void terminal_clear();
void terminal_refresh();

#endif
