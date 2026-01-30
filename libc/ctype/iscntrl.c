#include <ctype.h>

int iscntrl(int c) {
	unsigned char uc = (unsigned char)c;
	if (uc < ' ' || uc == 0x7f)
		return 1;
	return 0;
}