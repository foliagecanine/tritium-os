#include <stdio.h>
#include <string.h>

__asm__("push %ecx; push %eax; call main");

uint8_t buf[513];

_Noreturn void main(uint32_t argc, char **argv) {
	terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY,VGA_COLOR_BLACK));
	printf("Argc: %d\n",argc);
	printf("Argv: %#\n",(uint64_t)argv);
	for (uint32_t i = 0; i < argc; i++) {
		printf("Argloc        : %#\n",(uint64_t)argv[i]);
		printf("Argument (cat): %s\n",argv[i]);
	}
	FILE f = fopen("A:/README.TXT","r");
	printf("%d bytes\n",(uint32_t)f.size);
	for (uint32_t i = 0; i < f.size; i+=512) {
		fread(&f,buf,i,512);
		//printf("%s",buf);
	}
	exit(0);
}