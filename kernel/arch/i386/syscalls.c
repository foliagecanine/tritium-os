#include <kernel/syscalls.h>

#define NUM_SYSCALLS	1

static void *syscalls[NUM_SYSCALLS] = {
	&printf
};

void run_syscall() {
	static int syscall_num;
	asm("movl %%eax,%0" : "=r"(syscall_num));
	
	if (syscall_num>=NUM_SYSCALLS)
		asm("iret"); //We don't want to accidentally give them kernel mode access, do we?
	
	void *function = syscalls[syscall_num];
	
	int retVal;
	//Call the function
	asm volatile("\
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
	iret;\
	" : "=a" (retVal) : "r"(function));
}

void new_syscall(uint8_t inum, uint32_t irq_fn) {
	idt_new_int_flags(inum,irq_fn, 0x60);
}

void init_syscalls() {
	//Install all the syscalls we need
	new_syscall(0x80,&run_syscall);
	kprint("Initialized Syscalls.");
}