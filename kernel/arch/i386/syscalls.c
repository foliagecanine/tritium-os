#include <kernel/syscalls.h>

#define NUM_SYSCALLS	10

extern void start_program(char *name);

static void *syscalls[NUM_SYSCALLS] = {
	&terminal_writestring, 	// 0
	&exec_syscall,			// 1
	&exit_program,			// 2
	&terminal_putentryat,	// 3
	&getchar,				// 4
	&getkey,				// 5
	&yield,					// 6
	&getpid,				// 7
	&free_pages,			// 8
	&terminal_option		// 9
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
	if (syscall_num==1)
		asm("nop");
	
	void *function = syscalls[syscall_num];
	
	int retVal;
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
	" : "=a" (retVal) : "r"(function),"m"(ebx),"m"(ecx),"m"(edx),"m"(esi),"m"(edi));
}

void new_syscall(uint8_t inum, uint32_t irq_fn) {
	idt_new_int_flags(inum,irq_fn, 0x60);
}

extern void run_syscall_asm;

void init_syscalls() {
	//Install all the syscalls we need
	new_syscall(0x80,(uint32_t)&run_syscall_asm);
	kprint("[INIT] Initialized Syscalls.");
}