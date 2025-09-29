#ifndef _STRING_H
#define _STRING_H 1

#include <sys/cdefs.h>

#include <stddef.h>

int    memcmp(const void *, const void *, size_t);
void * memcpy(void *__restrict, const void *__restrict, size_t);
void * memmove(void *, const void *, size_t);
void * memset(void *, int, size_t);
size_t strlen(const char *);
int    strcmp(const char *s1, const char *s2);
char * strchr(const char *s, int c);
char * strrchr(const char *s, int c);
char * strcpy(char *dest, const char *src);
void   strcut(char *strfrom, char *strto, int from, int to);

#endif
