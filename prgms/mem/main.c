#include <stdio.h>
#include <string.h>
#include <sys.h>

void main() {
	terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
	uint32_t mem_pages = _syscall0(8);
	uint64_t t_mem = mem_pages;
	t_mem *= 4;
	printf("Memory available: %u KiB (%u MiB)\n", (uint32_t)(t_mem), (uint32_t)(t_mem / 1024));
}
