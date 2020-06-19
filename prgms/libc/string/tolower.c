#include <string.h>

char tolower(char c) {
	if (c>('A'-1)&&c<('Z'+1))
		return c+('a'-'A');
	return c;
}