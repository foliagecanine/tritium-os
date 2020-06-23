#define uint32_t unsigned long

__asm__("jmp main");

static inline void syscall(unsigned int syscall_num) {
	asm volatile("mov %0,%%eax;int $0x80"::"r"(syscall_num));
}

void writestring(char *string) {
	asm volatile("mov %0,%%ebx"::"r"(string));
	syscall(0);
}

uint32_t exec(char *name) {
	uint32_t retval;
	asm volatile("mov %0,%%ebx; mov $0,%%ecx; mov $0,%%edx; mov $0,%%esi; mov $0,%%edi"::"r"(name));
	syscall(1);
	asm volatile("mov %%eax,%0":"=m"(retval):);
	return retval;
}

void exit(int retval) {
	asm volatile("mov %0,%%ebx"::"r"(retval));
	syscall(2);
}

_Noreturn void main() {
	writestring("Hello from EXEC.PRG!\n");
	writestring("I'm going to launch TESTPRG.PRG now.\n");
	exec("A:/PRGMS/TESTPRG2.PRG");
	writestring("EXEC.PRG will now exit. Bye!\n");
	exit(0);
	for(;;);
}
