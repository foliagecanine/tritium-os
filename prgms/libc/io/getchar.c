#include <stdio.h>

char getchar() {
	char retval;
	asm volatile("mov $4,%%eax; int $0x80; mov %%al, %0":"=r"(retval):);
	return retval;
}

unsigned int getkey() {
	unsigned int retval;
	asm volatile("mov $5,%%eax; int $0x80; mov %%eax, %0":"=r"(retval):);
	return retval;
}