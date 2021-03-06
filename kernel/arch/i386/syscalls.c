#include <kernel/syscalls.h>

#define NUM_SYSCALLS	27

extern void start_program(char *name);
void fopen_usermode(FILE *f, const char* filename, const char* mode);
uint8_t fread_usermode(FILE *f, char *buf, uint32_t starth, uint32_t startl, uint32_t lenl);
uint8_t fwrite_usermode(FILE *f, char *buf, uint32_t starth, uint32_t startl, uint32_t lenl);
void readdir_usermode(FILE *f, FILE *o, char *buf, uint32_t n);
void fcreate_usermode(char *filename, FILE *o);
uint8_t fdelete_usermode(char *filename);
uint8_t ferase_usermode(char *filename);
void debug_break();
void *map_mem(void *address);
void null_function();

static void *syscalls[NUM_SYSCALLS] = {
	&terminal_writestring, 	// 0
	&exec_syscall,			// 1
	&exit_program,			// 2
	&terminal_putentryat,	// 3
	&getchar,				// 4
	&get_kbddata,			// 5
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
	&ferase_usermode,		// 17
	&readdir_usermode,		// 18
	&null_function,			// 19 (reserved for future file-related functions)
	&null_function,			// 20 (reserved for future file-related functions)
	&null_function,			// 21 (reserved for future file-related functions)
	&null_function,			// 22 (reserved for future file-related functions)
	&debug_break,			// 23
	&get_ticks,				// 24
	&fork_process,			// 25
	&map_mem,				// 26
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
	for (void *i = addr; i < addr+size; i+=4096) {
		if (!check_user(i)) {
			dprintf("%p is not within range\n",i);
			return false;
		}
	}
	return true;
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

uint8_t ferase_usermode(char *filename) {
	if (check_range(filename,strlen(filename))) {
		return ferase(filename);
	}
	return UNKNOWN_ERROR;
}

void debug_break() {
	asm volatile("nop");
}

void *map_mem(void *address) {
	if (address && address < (void *)0xC0000000) { // Don't map nullptrs or kernel memory.
		void *retval = map_page_to(address);
		memset(retval,0,4096);
		mark_user(retval,true);
		return retval;
	}
	return 0;
}

void null_function() {

}