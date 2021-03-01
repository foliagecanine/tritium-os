#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint8_t buf[513];

int main(uint32_t argc, char *argv[]) {
	terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY,VGA_COLOR_BLACK));
	FILE *fp;
	if (argc<2) {
		char *cd = getenv("CD");
		if (!cd) {
			printf("Error: no current directory.\n");
			return 1;
		}
		fp = fopen(cd,"r");
	} else {
		fp = fopen(argv[1],"r");		
	}
	
	if (fp->valid) {
		if (fp->directory) {
			FILE *r = fp;
			for (uint8_t i = 0; i<16; i++) {
				r = readdir(fp,buf,i);
				if (r->valid&&buf[0])
					printf("%s\n",buf);
				memset(buf,0,512);
			}
		} else {
			printf("Error: not a directory.\n");
			return 1;
		}
	} else {
		printf("Error: could not open directory.\n");
		return 1;
	}
	return 0;
}
