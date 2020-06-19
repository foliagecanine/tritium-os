#include <stdio.h>
#include <string.h>
#include <vga.h>

__asm__("jmp main");

_Noreturn void main() {
	uint32_t mem_pages = 0;
	terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY,VGA_COLOR_BLACK));
	asm("mov $8,%%eax;int $0x80;mov %%eax,%0":"=m"(mem_pages));
	printf("Memory available: %d KiB (%d MiB)\n",(mem_pages*4096)/1024,(mem_pages*4096)/1048576);
	asm("mov $2,%eax;int $0x80");
}