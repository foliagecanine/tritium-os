#ifndef _KERNEL_SERIAL_H
#define _KERNEL_SERIAL_H

void serial_init();
void serial_putchar(char c);
void serial_write(const char *str);
int dprintf(const char *restrict format, ...) __attribute__((format(printf, 1, 2)));
int noprintf(const char *restrict format, ...) __attribute__((format(printf, 1, 2)));

#endif // _KERNEL_SERIAL_H