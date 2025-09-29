#ifndef _KERNEL_SYSCALLS_H
#define _KERNEL_SYSCALLS_H

#include <kernel/stdio.h>
#include <kernel/idt.h>
#include <kernel/sysfunc.h>
#include <kernel/graphics.h>
#include <kernel/ipc.h>

void init_syscalls();

#endif
