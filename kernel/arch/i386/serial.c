#include <kernel/stdio.h>
#include <kernel/ksetup.h>

void serial_write(char *msg) {
	for (size_t l = 0; l< strlen(msg); l++) {
		while (!(inb(0x3fd)&0x20))
			;
		outb(0x3f8,msg[l]);
	}
}

void serial_init() {
	outb(0x3f9,0);
	outb(0x3fb,0x80);
	outb(0x3f8,3);
	outb(0x3f9,0);
	outb(0x3fb,3);
	outb(0x3fa,0xC7);
	outb(0x3fc,11);
	serial_write("Serial initialized.\n");
}