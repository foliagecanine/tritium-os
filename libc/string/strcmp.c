#include <string.h>

_Bool strcmp(const char *s1, const char *s2) {
	if (strlen(s1)!=strlen(s2))
		return 0;
	size_t amt = (strlen(s1) < strlen(s2)) ? strlen(s1) : strlen(s2);
	for (size_t len = 0; len<amt;len++)
		if (s1[len]!=s2[len]) return 0;
	return 1;
}
