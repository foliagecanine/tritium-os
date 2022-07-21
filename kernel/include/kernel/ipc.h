#ifndef _KERNEL_IPC_H
#define _KERNEL_IPC_H

#include <kernel/stdio.h>

#define IPC_MAX_RESPONSES (4096 / (sizeof(void*) + sizeof(uint16_t) + sizeof(size_t) + sizeof(bool)))

typedef struct ipc_response {
	uint16_t port;
	size_t 	 size;
	void 	 *phys_data;
} __attribute__((packed)) ipc_response;

#include <kernel/task.h>

bool init_ipc(void);
bool register_ipc_port(uint16_t port);
bool deregister_ipc_port(uint16_t port, bool override);
void deregister_ipc_ports_pid(uint32_t pid);
uint8_t transfer_ipc(uint16_t port, void *data, size_t size);
size_t receive_ipc_size(uint16_t port, bool override);
uint8_t receive_ipc(uint16_t port, void *data);
bool verify_ipc_port_ownership(uint32_t port);
bool transfer_avail(uint32_t pid, uint32_t port);
uint8_t respond_ipc(uint16_t port, void *data, size_t size);
size_t receive_response_size(void);
int32_t receive_response_ipc(void *data);
void reclaim_ipc_response_buffer(uint32_t pid, ipc_response *ir);

#endif
