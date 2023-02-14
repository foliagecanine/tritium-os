#include <ctype.h>

int islower(int c) {
	unsigned char uc = (unsigned char)c;
	if (uc >= 'a' && uc <= 'z')
		return 1;
	return 0;
}