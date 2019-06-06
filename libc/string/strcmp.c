#include <string.h>

_Bool strcmp(const char *s1, const char *s2) {
	if (strlen(s1)!=strlen(s2))
		return 0;
	for (size_t len = 0; len<strlen(s1);len++)
		if (s1[len]!=s2[len]) return 0;
	return 1;
}