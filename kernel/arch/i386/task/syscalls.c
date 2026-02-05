#include <kernel/syscalls.h>
#include <kernel/pmm.h>
#include <kernel/task.h>
#include <kernel/tty.h>
#include <kernel/kbd.h>
#include <kernel/ipc.h>
#include <kernel/graphics.h>
#include <fs/fs.h>

#define NUM_SYSCALLS	39

bool safe_dereg_ipc_port(uint16_t port);
size_t safe_receive_ipc_size(uint16_t port);
uint8_t safe_transfer_ipc(uint16_t port, void *data, size_t size);
uint8_t safe_receive_ipc(uint16_t port, void *data);

void fopen_usermode(FILE *f, const char* filename, const char* mode);
uint8_t fread_usermode(FILE *f, char *buf, uint32_t starth, uint32_t startl, uint32_t lenl);
uint8_t fwrite_usermode(FILE *f, char *buf, uint32_t starth, uint32_t startl, uint32_t lenl);
void readdir_usermode(FILE *f, FILE *o, char *buf, uint32_t n);
void fcreate_usermode(char *filename, FILE *o);
uint8_t fdelete_usermode(char *filename);
uint8_t fmove_usermode(char *src_filename, char *dest_filename);

uint32_t get_input_data(uint8_t input);

void *map_mem(void *address);

uint32_t graphics_function(uint8_t function, uint32_t param1, uint32_t param2, uint32_t param3);

void null_function();
void debug_break();

static void *syscalls[NUM_SYSCALLS] = {
	&terminal_writestring, 	// 0
	&exec_syscall,			// 1
	&exit_program,			// 2
	&terminal_putentryat,	// 3
	&getchar,				// 4
	&get_input_data,		// 5
	&yield,					// 6
	&getpid,				// 7
	&free_pages,			// 8
	&terminal_option,		// 9
	&waitpid,				// 10
	&get_retval,			// 11
	&fopen_usermode,		// 12
	&fread_usermode,		// 13
	&fwrite_usermode,		// 14
	&fcreate_usermode,		// 15
	&fdelete_usermode,		// 16
	&fmove_usermode,		// 17
	&readdir_usermode,		// 18
	&null_function,			// 19 (reserved for future file-related functions)
	&null_function,			// 20 (reserved for future file-related functions)
	&null_function,			// 21 (reserved for future file-related functions)
	&null_function,			// 22 (reserved for future file-related functions)
	&debug_break,			// 23
	&get_ticks,				// 24
	&fork_process,			// 25
	&map_mem,				// 26
	&graphics_function,		// 27
	&null_function, //register_ipc_port,		// 28
	&safe_dereg_ipc_port, 	// 29
	&safe_receive_ipc_size,	// 30
	&safe_transfer_ipc,		// 31
	&safe_receive_ipc,		// 32
	&waitipc,				// 33
	&null_function,			// 34 (reserved for future IPC-related functions)
	&null_function,			// 35 (reserved for future IPC-related functions)
	&null_function,			// 36 (reserved for future IPC-related functions)
	&null_function,			// 37 (reserved for future IPC-related functions)
	&serial_putchar			// 38
};

extern bool ts_enabled;

void run_syscall() {
	ts_enabled = false;
	static int syscall_num;
	static int ebx;
	static int ecx;
	static int edx;
	static int esi;
	static int edi;
	static int ebp;
	static int esp;
	asm("movl %%eax,%0" : "=m"(syscall_num));
	asm("movl %%ebx,%0" : "=m"(ebx));
	asm("movl %%ecx,%0" : "=m"(ecx));
	asm("movl %%edx,%0" : "=m"(edx));
	asm("movl %%esi,%0" : "=m"(esi));
	asm("movl %%edi,%0" : "=m"(edi));
	asm("movl %%ebp,%0" : "=m"(ebp));
	asm("movl %%esp,%0" : "=m"(esp));

	if (syscall_num>=NUM_SYSCALLS) {
		ts_enabled = true;
		return; //We don't want to accidentally give them kernel mode access, do we?
	}

	if (syscall_num==23)
		asm("nop");

	void *function = syscalls[syscall_num];

	int retval;
	ts_enabled = true;
	//Call the function
	asm volatile("\
	push %%esp;	\
	push %6; \
	push %5; \
	push %4; \
	push %3; \
	push %2; \
	call *%1; \
	pop %%ebx; \
	pop %%ebx; \
	pop %%ebx; \
	pop %%ebx; \
	pop %%ebx; \
	pop %%ebx; \
	ret;\
	" : "=a" (retval) : "r"(function),"m"(ebx),"m"(ecx),"m"(edx),"m"(esi),"m"(edi));
}

void new_syscall(uint8_t inum, uint32_t irq_fn) {
	idt_new_int_flags(inum,irq_fn, 0x60);
}

extern uint32_t run_syscall_asm;

void init_syscalls() {
	//Install all the syscalls we need
	new_syscall(0x80,(uint32_t)&run_syscall_asm);
	kprint("[INIT] Initialized Syscalls.");
}

// Make sure the program is not writing to kernel memory.
bool check_range(void *addr, uint32_t size) {
	for (void *i = addr; i < addr + size; i += PAGE_SIZE) {
		if (!(get_page_permissions(i) & PAGE_PERM_USER)) {
			dprintf("%p is not within range\n",i);
			return false;
		}
	}
	return true;
}

uint32_t get_input_data(uint8_t input) {
	switch(input) {
		case 0:
			return get_kbddata();
		case 1:
			return get_mousedataX();
		case 2:
			return get_mousedataY();
		case 3:
			return get_mousedataZ();
		case 4:
			return get_mousedataB();
		default:
			return 0;
	}
}

void fopen_usermode(FILE *f, const char* filename, const char* mode) {
	if (check_range(f,sizeof(FILE))) {
		*f = fopen(filename,mode);
	}
}

uint8_t fread_usermode(FILE *f, char *buf, uint32_t starth, uint32_t startl, uint32_t lenl) {
	uint64_t start = (((uint64_t)starth)<<32)|startl;
	uint64_t len = (uint64_t)lenl;
	if (check_range(buf,len)) {
		return fread(f,buf,start,len);
	}
	return UNKNOWN_ERROR;
}

uint8_t fwrite_usermode(FILE *f, char *buf, uint32_t starth, uint32_t startl, uint32_t lenl) {
	uint64_t start = (((uint64_t)starth)<<32)|startl;
	uint64_t len = (uint64_t)lenl;
	if (check_range(buf,len)) {
		return fwrite(f,buf,start,len);
	}
	return UNKNOWN_ERROR;
}

void readdir_usermode(FILE *f, FILE *o, char *buf, uint32_t n) {
	if (!check_range(f,sizeof(FILE)))
		return;
	if (!f->directory || !f->valid)
		return;
	if (check_range(buf,256)&&check_range(o,sizeof(FILE))) {
		*o = readdir(f, buf, n);
	}
}

void fcreate_usermode(char *filename, FILE *o) {
	if (filename) {
		if (check_range(filename,strlen(filename))&&check_range(o,sizeof(FILE))) {
			*o = fcreate(filename);
		}
	}
}

uint8_t fdelete_usermode(char *filename) {
	if (check_range(filename,strlen(filename))) {
		return fdelete(filename);
	}
	return UNKNOWN_ERROR;
}

uint8_t fmove_usermode(char *filename, char *dest) {
	if (check_range(filename,strlen(filename)) && (check_range(dest, strlen(dest)) || dest==NULL)) {
		return fmove(filename, dest);
	}
	return UNKNOWN_ERROR;
}

void debug_break() {
	asm volatile("nop");
}

void *map_mem(void *address) {
	address = (void *)((uint32_t)address & ~(PAGE_SIZE - 1)); // Align to 4KiB page
	if (address >= USER_MEM_START && address < USER_MEM_END) { // Don't map nullptrs or kernel memory.
		if (!map_user_page(address)) {
			return NULL;
		}

		memset(address, 0, PAGE_SIZE);
		return address;
	}
	return 0;
}

uint32_t graphics_function(uint8_t function, uint32_t param1, uint32_t param2, uint32_t param3) {
	switch (function) {
		case 0:
			return request_graphics_lock();
		case 1:
			return release_graphics_lock();
		case 2:
			return set_resolution(param1, param2, param3).raw;
		case 3:
			if (!check_range((void *)param1, get_framebuffer_size()))
				return 99;
			return copy_framebuffer((void *)param1);
		case 4:
			return set_text_mode();
		default:
			return 0;
	}

}

bool safe_dereg_ipc_port(uint16_t port) {
	(void)port;
	return false; //deregister_ipc_port(port, false);
}

size_t safe_receive_ipc_size(uint16_t port) {
	(void)port;
	return 0; //receive_ipc_size(port, false);
}

uint8_t safe_transfer_ipc(uint16_t port, void *data, size_t size) {
	(void)port;
	(void)data;
	(void)size;
	// if (!check_range(data, size))
	// 	return 4; // BAD_INPUT
	return 4; // transfer_ipc(port, data, size);
}

uint8_t safe_receive_ipc(uint16_t port, void *data) {
	(void)port;
	(void)data;
	// if (!check_range(data, receive_ipc_size(port, true)))
	// 	return 4; // BAD_INPUT
	return 4; // receive_ipc(port, data);
}

void null_function() {
	asm volatile("nop");
}
