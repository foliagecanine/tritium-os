#ifndef _KERNEL_TASK_H
#define _KERNEL_TASK_H

#include <fs/file.h>
#include <kernel/tss.h>
#include <kernel/ksetup.h>
#include <kernel/stdio.h>
#include <kernel/elf.h>
#include <kernel/mem.h>

#define STACK_START_ADDR ((void *)0xBFFFA000UL)
#define STACK_END_ADDR ((void *)0xBFFFD000UL)
#define ARGL_ADDR ((void *)0xBFFFE000UL)
#define ENVL_ADDR ((void *)0xBFFFF000UL)

#define IPC_MAX_RESPONSES (4096 / (sizeof(void*) + sizeof(uint16_t) + sizeof(size_t) + sizeof(bool)))

typedef struct ipc_response {
	uint16_t port;
	size_t 	 size;
	void 	 *phys_data;
} __attribute__((packed)) ipc_response;

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
page_directory_t *get_task_tables(uint32_t pid);

#endif
