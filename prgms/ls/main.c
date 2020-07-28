#include <stdio.h>
#include <string.h>

uint8_t buf[513];
char fname[4096];

void main(uint32_t argc, char **argv) {
	terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY,VGA_COLOR_BLACK));
	char *cd = getenv("CD");
	memset(fname,0,4096);
	FILE f;
	if (argc>1) {
		if (argv[1][1]==':')
			strcpy(fname,argv[1]);
		else {
			memcpy(fname,cd,strlen(cd));
			memcpy(fname+strlen(cd),argv[1],strlen(argv[1]));
		}
	} else {
		strcpy(fname,cd);
	}
	f = fopen(fname,"r");
	FILE r;
	r.valid = true;
	if (f.valid) {
		if (f.directory) {
			for (uint8_t i = 0; i<16&&r.valid; i++) {
				r = readdir(&f,buf,i);
				if (r.valid&&buf[0])
					printf("%s\n",buf);
				memset(buf,0,512);
			}
		} else {
			printf("Error: %s is not a directory.\n",fname);
		}
	} else {
		printf("Error: file %s not found.\n",fname);
	}
	exit(0);
}
