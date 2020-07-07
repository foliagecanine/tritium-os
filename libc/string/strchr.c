#include <string.h>

char* strchr(const char *s, int c)
{
	while ((*s) != (char)c&&(*s)!=0)
		if (!(*s++)) return 0;
	return (char *)s;
}