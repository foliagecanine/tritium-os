
#include <stdio.h>

int puts(const char *str) {
	while (*str) {
		if (putchar(*str++) == EOF)
			return EOF;
	}

	if (putchar('\n') == EOF)
		return EOF;

	return 0;
}
