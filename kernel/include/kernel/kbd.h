#ifndef _KERNEL_KBD_H
#define _KERNEL_KBD_H

#include <kernel/stdio.h>
//#include <kernel/kbd.h>
#include <kernel/exceptions.h>
//#include <kernel/pit.h>

unsigned int getkey();
int get_raw_scancode();
char getchar();
void print_keys();
char scancode_to_char(unsigned int scancode);
void kbd_handler();

#endif
