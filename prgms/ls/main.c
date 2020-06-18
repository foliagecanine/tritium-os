#include <stdio.h>
#include <string.h>

__asm__("push %ecx; push %eax; call main");

uint8_t buf[513];

_Noreturn void main(uint32_t argc, char **argv) {
	terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY,VGA_COLOR_BLACK));
	FILE f = fopen(argv[1],"r");
	FILE r = f;
	if (f.valid) {
		if (f.directory) {
			for (uint8_t i = 0; i<16&&r.valid; i++) {
				r = readdir(&f,buf,i);
				if (r.valid&&buf[0])
					printf("%s\n",buf);
				memset(buf,0,512);
			}
		} else {
			printf("Error: %s is not a directory.\n",argv[1]);
		}
	} else {
		printf("Error: file not found.\n");
	}
	exit(0);
}