#include <stdio.h>
#include <string.h>

void main() {
	uint32_t mem_pages = 0;
	terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
	asm("mov $8,%%eax;int $0x80;mov %%eax,%0" : "=m"(mem_pages));
	uint64_t t_mem = mem_pages;
	t_mem *= 4096;
	printf("Memory available: %u KiB (%u MiB)\n", (uint32_t)(t_mem / 1024), (uint32_t)(t_mem / 1048576));
}
