#include <string.h>

char toupper(char c) {
	if (c>('a'-1)&&c<('z'+1))
		return c-'a'+'A';
	return c;
}