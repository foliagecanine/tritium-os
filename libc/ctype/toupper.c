#include <ctype.h>

int toupper(int c) {
	if (islower(c))
		return c-'a'+'A';
	return c;
}