#include <stdio.h>

#if defined(__is_libk)
#include <kernel/tty.h>
#else
#include <tty.h>
#endif

int putchar(int ic) {
#if defined(__is_libk)
	char c = (char) ic;
	terminal_write(&c, sizeof(c));
#else
	char c = (char) ic;
	terminal_putchar(c);
#endif
	return ic;
}
