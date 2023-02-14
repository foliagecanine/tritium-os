#ifndef _STDLIB_H
#define _STDLIB_H 1

#include <sys/cdefs.h>

__attribute__((__noreturn__)) void abort(void);
long                               strtol(const char *str, char **endptr, int base);

#endif
