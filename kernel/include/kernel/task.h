#ifndef _KERNEL_TASK_H
#define _KERNEL_TASK_H

#include <fs/file.h>
#include <kernel/tss.h>
#include <kernel/ksetup.h>
#include <kernel/stdio.h>
#include <kernel/elf.h>

void task_switch(tss_entry_t tss);
void task_set_kdir(uint32_t kdir);
void create_process(void *prgm,size_t size);
void create_idle_process(void *prgm, size_t size);
void init_tasking(uint32_t num_pages);
void start_program(char *name);
void exit_program(int retval);
void kill_program(uint32_t pid, uint32_t retval);
uint32_t fork_process();

#endif