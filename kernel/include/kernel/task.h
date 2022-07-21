#ifndef _KERNEL_TASK_H
#define _KERNEL_TASK_H

#include <fs/file.h>
#include <kernel/tss.h>
#include <kernel/ksetup.h>
#include <kernel/stdio.h>
#include <kernel/elf.h>
#include <kernel/ipc.h>

typedef struct ipc_response ipc_response;

void task_switch(tss_entry_t tss);
void create_process(void *prgm,size_t size);
void create_idle_process(void *prgm, size_t size);
void init_tasking(uint32_t num_pages);
void start_program(char *name);
void exit_program(int retval);
void kill_process(uint32_t pid, uint32_t retval);
uint32_t fork_process(void);
void waitipc(uint32_t port);
ipc_response *get_ipc_responses(uint32_t pid);
int get_ipc_response_status(uint32_t pid, bool consumed);
bool set_ipc_response_status(uint32_t pid, bool consumed, uint16_t value);

#endif
