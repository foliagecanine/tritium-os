#include <kernel/ipc.h>

#define IPC_MAX_PORTS 65536

typedef struct {
	uint32_t pid;
	size_t size;
	void * data;
} ipc_message;

#define IPC_MAX_MESSAGES (4096 / sizeof(ipc_message))

typedef struct {
	uint32_t      pid;
	ipc_message **messages;
	void 		 *phys_messages;
	uint16_t	  responded;
	uint16_t      consumed;
	uint16_t      produced;
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
	ipc_ports[port].messages = alloc_page(1);
	if (!ipc_ports[port].messages)
		return false;
	ipc_ports[port].pid = getpid();
	ipc_ports[port].phys_messages = get_phys_addr(ipc_ports[port].messages);
	ipc_ports[port].responded = 0;
	ipc_ports[port].consumed = 0;
	ipc_ports[port].produced = 0;
	memset(ipc_ports[port].messages, 0, 4096);

	return true;
}

uint16_t wrap_increment_message(uint16_t v) {
	if (v > IPC_MAX_MESSAGES)
		return 0;
	return v + 1;
}

ipc_message *next_message(uint16_t port) {
	if (ipc_ports[port].consumed == ipc_ports[port].produced)
		return 0;

	return ((void *)ipc_ports[port].messages) + (wrap_increment_message(ipc_ports[port].consumed) * sizeof(ipc_message));
}

bool deregister_ipc_port(uint16_t port, bool override) {
	if (override || !getpid() || getpid() == ipc_ports[port].pid) { // Make sure program deregistering actually owns it (or is the kernel)
		ipc_ports[port].pid = 0;
		ipc_message *m = next_message(port);
		ipc_ports[port].consumed = wrap_increment_message(ipc_ports[port].consumed);
		while(m) {
			for (size_t i = 0; i < (m->size + 4095) / 4096; i++) {
				release_phys_page(m->data + (i * 4096)); // This memory is already unmapped, just set it as usable.
			}
			m = next_message(port);
			ipc_ports[port].consumed = wrap_increment_message(ipc_ports[port].consumed);
		}
		free_page(ipc_ports[port].messages, 1);
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
	if (size == 0)
		return 0; // We did it! We successfully moved zero bytes.
	if (ipc_ports[port].pid == 0)
		return 1; // PORT_NOEXIST
	if (wrap_increment_message(ipc_ports[port].produced) == ipc_ports[port].consumed) // Ensure we don't completely wrap around
		return 2; // PORT_BUSY
	if (wrap_increment_message(ipc_ports[port].produced) == ipc_ports[port].responded) // Disallow transfer if we will corrupt the responded pointer
		return 4; // RESPONSE_REQUIRED
	uint32_t num_pages = (size + 4095) / 4096;
	void *   new_data = alloc_sequential(num_pages); // Unfortunately since we are in a different address space, it must be sequential so we can trade the physical address
	if (!new_data)
		return 3; // OUT_OF_MEMORY
	memcpy(new_data, data, size);
	ipc_ports[port].produced = wrap_increment_message(ipc_ports[port].produced);
	ipc_message **messages = map_paddr(ipc_ports[port].phys_messages, 1); // Since the page is registered to another process' address space we need to add it to ours temporarily
	ipc_message *m = ((void *)messages) + (ipc_ports[port].produced * sizeof(ipc_message));
	m->data = get_phys_addr(new_data);
	m->size = size;
	m->pid = getpid();
	trade_vaddr(new_data);
	trade_vaddr(messages); // Remove the messages page from our address space
	return 0;
}

size_t receive_ipc_size(uint16_t port, bool override) {
	if (override || !getpid() || getpid() == ipc_ports[port].pid) {
		if (ipc_ports[port].produced == ipc_ports[port].consumed)
			return 0;
		return next_message(port)->size;
	}

	return 0;
}

bool verify_ipc_port_ownership(uint32_t port) {
	if (port == (uint32_t)-1) { // Check to see if this process owns ANY ports
		for (uint32_t i = 0; i < IPC_MAX_PORTS; i++) {
			if (getpid() == ipc_ports[i].pid)
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
		for (uint32_t i = 0; i < IPC_MAX_PORTS; i++) {
			if (pid == ipc_ports[i].pid && ipc_ports[i].produced != ipc_ports[i].consumed)
				return true;
		}
		return false;
	}

	if (port > 0xFFFF)
		return false;

	if (pid == ipc_ports[port].pid && ipc_ports[port].produced != ipc_ports[port].consumed)
		return true;

	return false;
}

uint8_t receive_ipc(uint16_t port, void *data) {
	if (getpid() != ipc_ports[port].pid)
		return 1; // WRONG_PID

	if (ipc_ports[port].produced == ipc_ports[port].consumed)
		return 2; // NO_DATA

	ipc_message *m = next_message(port);
	ipc_ports[port].consumed = wrap_increment_message(ipc_ports[port].consumed);

	uint32_t num_pages = (m->size + 4095) / 4096;
	void *   new_data = map_paddr(m->data, num_pages);
	memcpy(data, new_data, m->size);

	free_page(new_data, num_pages);

	m->data = 0;
	m->size = 0;
	m->pid = 0;

	return 0;
}

uint16_t wrap_increment_response(uint16_t v) {
	if (v > IPC_MAX_RESPONSES)
		return 0;
	return v + 1;
}

ipc_response *empty_response(uint32_t pid) {
	ipc_response *ir = get_ipc_responses(pid);
	ir += get_ipc_response_status(pid, false);
	return ir;
}

uint8_t respond_ipc(uint16_t port, void *data, size_t size) {
	if (getpid() != ipc_ports[port].pid)
		return 1; // WRONG_PID

	if (ipc_ports[port].consumed == ipc_ports[port].responded)
		return 2; // NO_DATA

	ipc_message *m = ((void *)ipc_ports[port].messages) + (wrap_increment_message(ipc_ports[port].responded) * sizeof(ipc_message));

	if (size != 0 && get_ipc_responses(m->pid) && get_ipc_response_status(m->pid, false) == wrap_increment_response(get_ipc_response_status(m->pid, true)))
		return 4; // RESPONSE_BUFFER_FULL

	ipc_ports[port].responded = wrap_increment_message(ipc_ports[port].responded);

	if (!get_ipc_responses(m->pid))
		return 5; // PROCESS_DIED

	if (size == 0)
		return 0; // Do not actually send 0 bytes.

	uint32_t num_pages = (size + 4095) / 4096;
	void *new_data = alloc_sequential(num_pages);

	if (!new_data)
		return 3; // OUT_OF_MEMORY

	memcpy(new_data, data, size);
	ipc_response *r = empty_response(m->pid);

	r->port = port;
	r->size = size;
	r->phys_data = get_phys_addr(new_data);

	set_ipc_response_status(m->pid, false, wrap_increment_response(get_ipc_response_status(m->pid, false)));

	return 0;
}

size_t receive_response_size() {
	if (get_ipc_response_status(getpid(),true) == get_ipc_response_status(getpid(),false))
		return 0;
	return get_ipc_responses(getpid())[get_ipc_response_status(getpid(),true)].size;
}

int32_t receive_response_ipc(void *data) {
	// TODO: This function is sad. Please make function happy.

	// :-(
}

void reclaim_ipc_response_buffer(uint32_t pid, ipc_response *ir) {
	uint16_t num_buffers = get_ipc_response_status(pid, false);
	for (size_t i = get_ipc_response_status(pid, true); i < num_buffers; i++) {
		if (!ir[i].phys_data)
			continue;
		for (size_t j = 0; j < (ir[i].size + 4095) / 4096; j++) {
				release_phys_page(ir[i].phys_data + (j * 4096));
		}
	}
	free_page(ir, 1);
}
