#include <ctype.h>

int isupper(int c) {
	unsigned char uc = (unsigned char)c;
	if (uc >= 'A' && uc <= 'Z')
		return 1;
	return 0;
}