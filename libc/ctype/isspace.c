#include <ctype.h>

int isspace(int c) {
	unsigned char uc = (unsigned char)c;
	if (uc == ' ' || uc == '\n' || uc == '\t' || uc == '\v' || uc == '\v' || uc == '\f' || uc == '\r')
		return 1;
	return 0;
}