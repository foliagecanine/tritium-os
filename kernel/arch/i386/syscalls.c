#include <kernel/syscalls.h>

#define NUM_SYSCALLS	3

extern void start_program(char *name);

static void *syscalls[NUM_SYSCALLS] = {
	&terminal_writestring,
	&start_program,
	&exit_program
};

extern bool ts_enabled;
uint32_t ready_esp;

void run_syscall() {
	ts_enabled = false;
	static int syscall_num;
	asm("movl %%eax,%0" : "=r"(syscall_num));
	
	if (syscall_num>=NUM_SYSCALLS) {
		ts_enabled = true;
		asm("iret"); //We don't want to accidentally give them kernel mode access, do we?
	}
	
	void *function = syscalls[syscall_num];
	
	int retVal;
	ts_enabled = true;
	//Call the function
	asm volatile("\
	push %%esp;	\
	push %%edi; \
	push %%esi; \
	push %%edx; \
	push %%ecx; \
	push %%ebx; \
	call *%1; \
	pop %%ebx; \
	pop %%ebx; \
	pop %%ebx; \
	pop %%ebx; \
	pop %%ebx; \
	pop %%ebx; \
	iret;\
	" : "=a" (retVal) : "r"(function),"r"(ready_esp));
}

void new_syscall(uint8_t inum, uint32_t irq_fn) {
	idt_new_int_flags(inum,irq_fn, 0x60);
}

void init_syscalls() {
	//Install all the syscalls we need
	new_syscall(0x80,(uint32_t)&run_syscall);
	kprint("[INIT] Initialized Syscalls.");
}