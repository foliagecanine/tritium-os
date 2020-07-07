#include <string.h>

char* strrchr(const char *s, int c)
{
	const char *ss = s;
	s +=strlen(ss)-1;
	while ((*s) != (char)c)
		if (s--<ss) return 0;
	return (char *)s;
}
