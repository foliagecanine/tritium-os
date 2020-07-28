#include <stdio.h>
#include <string.h>

uint8_t buf[513];
char fname[4096];

void main(uint32_t argc, char **argv) {
	terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY,VGA_COLOR_BLACK));
	if (argc<2)
		exit(1);
	char *cd = getenv("CD");
	memset(fname,0,4096);
	FILE f;
	if (argv[1][1]==':')
		strcpy(fname,argv[1]);
	else {
		memcpy(fname,cd,strlen(cd));
		memcpy(fname+strlen(cd),argv[1],strlen(argv[1]));
	}
	f = fopen(fname,"r");
	if (f.valid) {
		if (!f.directory) {
			printf("%d bytes\n",(uint32_t)f.size);
			for (uint32_t i = 0; i < f.size; i+=512) {
				fread(&f,buf,i,512);
				printf("%s",buf);
			}
		} else {
			printf("Error: %s is directory.\n",fname);
		}
	} else {
		printf("Error: file %s not found.\n",fname);
	}
	exit(0);
}
