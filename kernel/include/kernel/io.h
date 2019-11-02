#ifndef _KERNEL_IO_H
#define _KERNEL_IO_H

#include <kernel/stdio.h>

void outb(uint16_t port, uint8_t val);
void outw(uint16_t port, uint16_t val);
void outl(uint16_t port, uint32_t val);

uint8_t inb(uint16_t port);
uint16_t inw(uint16_t port);
uint32_t inl(uint16_t port);

void insb(unsigned short port, unsigned char * data, unsigned long size);
void insw(unsigned short port, unsigned char * data, unsigned long size);
void insl(unsigned short port, unsigned char * data, unsigned long size);

int cpuid_string(int code, int where[4]);
char * cpu_string();
#endif