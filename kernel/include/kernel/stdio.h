#ifndef _KERNEL_STDIO_H
#define _KERNEL_STDIO_H

// These includes are here because they are deemed neccesary for most functions of the kernel.
#include <stdio.h>         // printf, etc.
#include <stddef.h>        //size_t, possibly sizeof, etc.
#include <stdint.h>        // int8_t, uint32_t, etc.
#include <kernel/io.h> //outb, inl, etc.
#include <kernel/kbd.h>
#include <kernel/kprint.h> //kprint, kerror, etc.
#include <stdlib.h>
#include <string.h> // memcpy, strlen, etc.

// There is no predefined values for true or false, so we'll do it here
#define false 0
#define true 1 // Can be anything but zero, we'll do one for the sake of simplicity

#define bool _Bool

void debug_console(); // Hopefully temporary
void serial_write(const char *msg);
int  dprintf(const char *restrict format, ...);
int  noprintf(const char *restrict format, ...);

#endif
