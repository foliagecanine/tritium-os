#ifndef _KERNEL_PIT_H
#define _KERNEL_PIT_H

#include <kernel/stdio.h>

void init_pit();
void pit_tick();
void sleep(uint32_t ms);

#endif