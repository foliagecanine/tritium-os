__asm__("jmp main");

static inline void syscall(unsigned int syscall_num) {
	asm volatile("mov %0,%%eax;int $0x80"::"r"(syscall_num));
}

void writestring(char *string) {
	asm volatile("mov %0,%%ebx"::"r"(string));
	syscall(0);
}

void exec(char *name) {
	asm volatile("mov %0,%%ebx; mov $0,ecx; mov $0,edx"::"r"(name));
	syscall(1);
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
