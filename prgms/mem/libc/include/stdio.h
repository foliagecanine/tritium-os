#ifndef _STDIO_H
#define _STDIO_H 1

#include <sys/cdefs.h>
#include <stdint.h>

#define EOF (-1)

#ifdef __cplusplus
extern "C" {
#endif

int printf(const char* __restrict, ...);
int putchar(int);
void terminal_clear();
void terminal_init();
char getchar();
unsigned int getkey();
void terminal_backup();
void terminal_setcolor(uint8_t color);

#ifdef __cplusplus
}
#endif

#endif
