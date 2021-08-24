#ifndef _KERNEL_KBD_H
#define _KERNEL_KBD_H

#include <kernel/stdio.h>
//#include <kernel/kbd.h>
#include <kernel/exceptions.h>
//#include <kernel/pit.h>

#define PS2_KEY_PRESSED 0
#define PS2_KEY_RELEASED 128

#define PS2_LSHIFT 42
#define PS2_RSHIFT 54
#define PS2_CTRL 29
#define PS2_ALT 56
#define PS2_CAPSLCK 58
#define PS2_SCRLCK 70
#define PS2_NUMLCK 69

unsigned int getkey();
int get_raw_scancode();
uint32_t get_kbddata();
char getchar();
void print_keys();
char scancode_to_char(unsigned int scancode);
void kbd_handler();
void insert_scancode(uint8_t scancode);
void mouse_handler();
void init_mouse();
void mouse_add_delta(int x, int y);
void mouse_buttons_override(uint8_t new_buttons);
int get_mousedataX();
int get_mousedataY();
int get_mousedataZ();
uint32_t get_mousedataB();

#endif
