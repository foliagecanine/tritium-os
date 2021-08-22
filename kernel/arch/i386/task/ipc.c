#include <kernel/ipc.h>

#define IPC_MAX_PORTS 65536

typedef struct {
	uint32_t pid;
	bool     xfer_avail;
	size_t   size;
	void *   data;
} ipc_port;

ipc_port *ipc_ports;

bool init_ipc() {
	ipc_ports = alloc_page(sizeof(ipc_port) * IPC_MAX_PORTS / 4096);
	if (ipc_ports == NULL)
		return false; // Error out if we can't allocate the memory to give more useful output to the user rather than a page fault
	memset(ipc_ports, 0, sizeof(ipc_port) * IPC_MAX_PORTS);
	kprint("[INIT] Initialized IPC");
	return true;
}

bool register_ipc_port(uint16_t port) {
	if (ipc_ports[port].pid != 0)
		return false; // Port already given to another process
	ipc_ports[port].pid = getpid();
	return true;
}

bool deregister_ipc_port(uint16_t port, bool override) {
	if (override || !getpid() || getpid() == ipc_ports[port].pid) { // Make sure program deregistering actually owns it (or is the kernel)
		ipc_ports[port].pid = 0;
		if (ipc_ports[port].xfer_avail) {
			if (ipc_ports[port].data != NULL) {
				uint32_t num_pages = (ipc_ports[port].size + 4095) / 4096; // Effectively round up to nearest multiple of 4096
				free_page(ipc_ports[port].data, num_pages);
			}
		}
		return true;
	}
	return false;
}

void deregister_ipc_ports_pid(uint32_t pid) {
	for (uint32_t i = 0; i < IPC_MAX_PORTS; i++) {
		if (ipc_ports[i].pid == pid) {
			deregister_ipc_port(i, true);
		}
	}
}

uint8_t transfer_ipc(uint16_t port, void *data, size_t size) {
	if (ipc_ports[port].pid == 0)
		return 1; // PORT_NOEXIST
	if (ipc_ports[port].xfer_avail)
		return 2; // PORT_BUSY
	uint32_t num_pages = (size + 4095) / 4096;
	void *new_data = alloc_sequential(num_pages); // Unfortunately since we are in a different address space, it must be sequential so we can trade the physical address
	if (!new_data)
		return 3; // OUT_OF_MEMORY
	memcpy(new_data, data, size);
	ipc_ports[port].data = get_phys_addr(new_data);
	ipc_ports[port].size = size;
	ipc_ports[port].xfer_avail = true;
	trade_vaddr(new_data);
	return 0;
}

size_t receive_ipc_size(uint16_t port, bool override) {
	if (override || !getpid() || getpid() == ipc_ports[port].pid) {
		return ipc_ports[port].size;
	}
	return -1;
}

bool verify_ipc_port_ownership(uint32_t port) {
	if (port == (uint32_t)-1) { // Check to see if this process owns ANY ports
		for (uint32_t i = 0; i < 65536; i++) {
			if (getpid() == ipc_ports[port].pid)
				return true;
		}
		return false;
	}
	if (port > 0xFFFF)
		return false;

	if (getpid() == ipc_ports[port].pid)
		return true;

	return false;
}

bool transfer_avail(uint32_t pid, uint32_t port) {
	if (port == (uint32_t)-1) {
		for (uint32_t i = 0; i < 65536; i++) {
			if (pid == ipc_ports[port].pid && ipc_ports[port].xfer_avail)
				return true;
		}
		return false;
	}

	if (port > 0xFFFF)
		return false;

	if (pid == ipc_ports[port].pid && ipc_ports[port].xfer_avail)
		return true;

	return false;
}

uint8_t receive_ipc(uint16_t port, void *data) {
	if (getpid() != ipc_ports[port].pid)
		return 1; // WRONG_PID

	if (!ipc_ports[port].xfer_avail)
		return 2; // NO_DATA

	uint32_t num_pages = (ipc_ports[port].size + 4095) / 4096;
	void *new_data = map_paddr(ipc_ports[port].data, num_pages);
	memcpy(data, new_data, ipc_ports[port].size);

	free_page(new_data, num_pages);

	return 0;
}
