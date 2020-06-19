#include <stdio.h>
#include <string.h>

char **envp;
uint32_t envc;

char *getenv(char *name) {
	for (uint32_t i = 0; i < envc; i+=2) {
		if (strcmp(envp[i],name))
			return envp[i+1];
	}
	return 0;
}