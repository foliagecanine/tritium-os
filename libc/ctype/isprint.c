#include <ctype.h>

int isprint(int c) {
	return !iscntrl(c);
}