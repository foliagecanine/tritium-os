#ifndef _STRING_H
#define _STRING_H 1

#include <sys/cdefs.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int memcmp(const void*, const void*, size_t);
void* memcpy(void* __restrict, const void* __restrict, size_t);
void* memmove(void*, const void*, size_t);
void* memset(void*, int, size_t);
size_t strlen(const char*);
_Bool strcmp(const char *s1, const char *s2);
char tolower(char c);
char toupper(char c);
char* strchr(const char *s, int c);
char *strcpy(char *dest, const char *src);
void strcut(char* strfrom, char* strto, int from, int to);

#ifdef __cplusplus
}
#endif

#endif
