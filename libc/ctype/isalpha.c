#include <ctype.h>

int isalpha(int c) {
	unsigned char uc = (unsigned char)c;
	if (uc >= 'A' && uc <= 'Z')
		return 1;
	if (uc >= 'a' && uc <= 'z')
		return 1;
	return 0;
}