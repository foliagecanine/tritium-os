#ifndef _KERNEL_PIT_H
#define _KERNEL_PIT_H

#include <kernel/stdio.h>

void init_pit();
void pit_tick();
uint32_t get_ticks(bool high);
uint64_t get_ticks_k();
void sleep(uint32_t ms);

#endif
