#ifndef _KERNEL_EXCEPTIONS_H
#define _KERNEL_EXCEPTIONS_H

#include <kernel/stdio.h>
#include <kernel/idt.h>
#include <kernel/sysfunc.h>
#include <kernel/mem.h>

void init_exceptions();

#endif