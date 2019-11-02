#include <kernel/power.h>

//void apmshutdown() is in powerasm.asm

void reboot() {
	outb(0xCF9,6);
}
