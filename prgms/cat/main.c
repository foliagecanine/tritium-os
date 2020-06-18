#include <stdio.h>
#include <string.h>

__asm__("push %ecx; push %eax; call main");

uint8_t buf[513];

_Noreturn void main(uint32_t argc, char **argv) {
	terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY,VGA_COLOR_BLACK));
	FILE f = fopen(argv[1],"r");
	if (f.valid) {
		if (!f.directory) {
			printf("%d bytes\n",(uint32_t)f.size);
			for (uint32_t i = 0; i < f.size; i+=512) {
				fread(&f,buf,i,512);
				printf("%s",buf);
			}
		} else {
			printf("Error: %s is directory.\n",argv[1]);
		}
	} else {
		printf("Error: file not found.\n");
	}
	exit(0);
}