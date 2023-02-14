#include <ctype.h>

int isdigit(int c) {
	unsigned char uc = (unsigned char)c;
	if (uc >= '0' && uc <= '9')
		return 1;
	return 0;
}