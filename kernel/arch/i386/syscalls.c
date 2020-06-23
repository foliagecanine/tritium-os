#include <kernel/syscalls.h>

#define NUM_SYSCALLS	16

extern void start_program(char *name);
void fopen_usermode(FILE *f, const char* filename, const char* mode);
void fread_usermode(FILE *f, char *buf, uint32_t starth, uint32_t startl, uint32_t lenl);
void readdir_usermode(FILE *f, FILE *o, char *buf, uint32_t n);
void debug_break();

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
	&terminal_option,		// 9
	&waitpid,				// 10
	&get_retval,			// 11
	&fopen_usermode,		// 12
	&fread_usermode,		// 13
	&readdir_usermode,		// 14
	&debug_break,			// 15
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

void fopen_usermode(FILE *f, const char* filename, const char* mode) {
	if ((uint32_t)f>0x100000&&(uint32_t)f+sizeof(FILE)<0xF04000) {
		*f = fopen(filename,mode);
	}
}

void fread_usermode(FILE *f, char *buf, uint32_t starth, uint32_t startl, uint32_t lenl) {
	uint64_t start = (((uint64_t)starth)<<32)|startl;
	uint64_t len = (uint64_t)lenl;
	if ((uint32_t)buf>0x100000&&(uint32_t)buf+len<0xF04000) {
		fread(f,buf,start,len);
	}
}

void readdir_usermode(FILE *f, FILE *o, char *buf, uint32_t n) {
	if (!f->directory)
		return;
	if ((uint32_t)buf>0x100000&&(uint32_t)buf+256<0xF04000&&(uint32_t)f>0x100000&&(uint32_t)f+256<0xF04000) {
		*o = readdir(f, buf, n);
	}
}

void debug_break() {
	asm volatile("nop");
}