#ifndef _STDIO_H
#define _STDIO_H 1

#include <stdarg.h>
#include <stddef.h>
// REMOVEME
#include <stdbool.h>

#define EOF (-1)
#define NULL ((void *)0)

int __print_formatted(int (*)(const char *, size_t), const char *, va_list);
int vprintf(const char *format, va_list arg);
int printf(const char *format, ...) __attribute__((format(printf, 1, 2)));
int putchar(int);
int puts(const char *);

#endif
