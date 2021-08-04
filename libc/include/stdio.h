#ifndef _STDIO_H
#define _STDIO_H 1

#include <sys/cdefs.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdbool.h>

#define EOF (-1)

#ifdef __cplusplus
extern "C" {
#endif

int __printf_template(bool (*)(const char *, size_t), const char* __restrict, va_list);
int printf(const char* __restrict, ...);
int putchar(int);
int puts(const char*);

#ifdef __cplusplus
}
#endif

#endif
