#include <kernel/stdio.h>
#include <kernel/ksetup.h>
#include <limits.h>
#include <stdarg.h>

void serial_write(const char *msg) {
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

static bool dprint(const char* data, size_t length) {
	for (size_t i = 0; i < length; i++) {
		while (!(inb(0x3fd)&0x20))
			;
		outb(0x3f8,data[i]);
	}
	return true;
}

int dprintf(const char* restrict format, ...) {
	va_list parameters;
	va_start(parameters, format);
	int ret = __printf_template(dprint,format,parameters);
	va_end(parameters);
	return ret;
}
