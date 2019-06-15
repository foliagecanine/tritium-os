#include <kernel/syscalls.h>

#define NUM_SYSCALLS	1

static void *syscalls[NUM_SYSCALLS] = {
	&printf
};

void run_syscall() {
	static int syscall_num;
	asm("movl %%eax,%0" : "=r"(syscall_num));
	
	if (syscall_num>=NUM_SYSCALLS)
		return;
	
	void *function = syscalls[syscall_num];
	
	int retVal;
	asm volatile("push %%edi\npush %%esi\npush %%edx\npush %%ecx\npush %%ebx\ncall *%1\npop %%ebx\npop %%ebx\npop %%ebx\npop %%ebx\npop %%ebx\nsti" : "=a" (retVal) : "r"(function));
}

void new_syscall(uint8_t inum, uint32_t irq_fn) {
	idt_new_int_flags(inum,irq_fn, 0x60);
}

void init_syscalls() {
	//Install all the syscalls we need
	new_syscall(0x80,&run_syscall);
	kprint("Initialized Syscalls.");
}