#include <stdio.h>
#include <stdlib.h>

uint8_t buf[513];

int main(int argc, char *argv[]) {
	terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY,VGA_COLOR_BLACK));
	if (argc<2)
		exit(1);
	FILE *fp = openfile(argv[1],"r");
	if (fp->valid) {
		if (!fp->directory) {
			printf("%u bytes\n",(uint32_t)fp->size);
			for (uint32_t i = 0; i < fp->size; i+=512) {
				if (readfile(fp,buf,i,512))
					return 1;
				printf("%s",buf);
			}
		} else {
			printf("Error: %s is directory.\n",argv[1]);
		}
	} else {
		printf("Error: file %s not found.\n",argv[1]);
	}
	closefile(fp);
}
