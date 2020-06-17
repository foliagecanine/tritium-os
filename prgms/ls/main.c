#include <stdio.h>
#include <string.h>

__asm__("jmp main");

uint8_t buf[513];

_Noreturn void main() {
	terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY,VGA_COLOR_BLACK));
	FILE f = fopen("A:/README.TXT","r");
	printf("%d bytes\n",(uint32_t)f.size);
	for (uint32_t i = 0; i < f.size; i+=512) {
		fread(&f,buf,i,512);
		printf("%s",buf);
	}
	exit(0);
}