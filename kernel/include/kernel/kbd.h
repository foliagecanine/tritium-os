#ifndef _KERNEL_KBD_H
#define _KERNEL_KBD_H

#include <kernel/stdio.h>

typedef struct {
	char charEquiv;
	_Bool released;
	uint8_t scanCode;
} kbdin;

void kbd_handler();
kbdin getInKey(_Bool shift);
char getchar();
unsigned int getkey();
void print_keys();
char scancode_to_char(unsigned int scancode);

#endif
