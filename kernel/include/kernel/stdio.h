#ifndef _KERNEL_STDIO_H
#define _KERNEL_STDIO_H

//These includes are here because they are deemed neccesary for most functions of the kernel.
#include <stdio.h> // printf, etc.
#include <stdint.h> // int8_t, uint32_t, etc.
#include <stddef.h> //size_t, possibly sizeof, etc.
#include <string.h> // memcpy, strlen, etc.
#include <stdlib.h>
#include <kernel/kprint.h> //kprint, kerror, etc.
#include <kernel/io.h> //outb, inl, etc.
#include <kernel/kbd.h>

//There is no predefined values for true or false, so we'll do it here
#define false 0
#define true 1 //Can be anything but zero, we'll do one for the sake of simplicity

#define bool _Bool

void debug_console(); //Hopefully temporary
void serial_write(const char *msg);

#endif
