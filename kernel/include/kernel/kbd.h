#ifndef _KERNEL_KBD_H
#define _KERNEL_KBD_H

#include <kernel/stdio.h>
#include <kernel/exceptions.h>

unsigned int getkey();
int get_raw_scancode();
uint32_t get_kbddata();
char getchar();
void print_keys();
char scancode_to_char(unsigned int scancode);
void kbd_handler();
void insert_scancode(uint8_t scancode);
void init_kbd();
void mouse_handler();
void init_mouse();
void mouse_add_delta(int x, int y);
void mouse_buttons_override(uint8_t new_buttons);
int get_mousedataX();
int get_mousedataY();
int get_mousedataZ();
uint32_t get_mousedataB();

#endif
