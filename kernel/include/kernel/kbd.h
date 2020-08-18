#ifndef _KERNEL_KBD_H
#define _KERNEL_KBD_H

#include <kernel/stdio.h>
//#include <kernel/kbd.h>
#include <kernel/exceptions.h>
//#include <kernel/pit.h>

unsigned int getkey();
int get_raw_scancode();
uint32_t get_kbddata();
char getchar();
void print_keys();
char scancode_to_char(unsigned int scancode);
void kbd_handler();
void mouse_handler();
void mouse_set_resolution(uint32_t width, uint32_t height);
void init_mouse();
uint32_t mouse_getx();
uint32_t mouse_gety();
void mouse_set_override(uint32_t _x, uint32_t _y);
//_Bool wait_mouse_pkt(uint8_t bit);

#endif
