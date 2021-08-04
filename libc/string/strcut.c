#include <string.h>

void strcut(char* strfrom, char* strto, int from, int to) {
	memcpy( strto, &strfrom[from], to-from );
}
