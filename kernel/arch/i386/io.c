#include <kernel/io.h>

// Based on https://wiki.osdev.org/Inline_Assembly/Examples

uint8_t inb(uint16_t port) {
	uint8_t ret;
	asm volatile("inb %%dx, %%al":"=a"(ret):"d"(port));
	return ret;
}

uint16_t inw(uint16_t port)
{
	uint16_t ret;
	asm volatile("inw %%dx, %%ax":"=a"(ret):"d"(port));
	return ret;
}

uint32_t inl(uint16_t port)
{
	uint32_t ret;
	asm volatile("inl %%dx, %%eax":"=a"(ret):"d"(port));
	return ret;
}

void outb(uint16_t port, uint8_t value)
{
	asm volatile("outb %%al, %%dx": :"d" (port), "a" (value));
}

void outw(uint16_t port, uint16_t value)
{
	asm volatile("outw %%ax, %%dx": :"d" (port), "a" (value));
}

void outl(uint16_t port, uint32_t value)
{
	asm volatile("outl %%eax, %%dx": :"d" (port), "a" (value));
}
