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
int getkey();

#endif