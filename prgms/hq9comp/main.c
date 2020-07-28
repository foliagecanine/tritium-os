#include <stdio.h>
#include <string.h>

uint8_t buf[513];
char fname[4096];
bool ignore = false;

void main(uint32_t argc, char **argv) {
	terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY,VGA_COLOR_BLACK));
	for (uint32_t i = 1; i < argc; i++) {
		if (strcmp(argv[i],"-?")||strcmp(argv[i],"--help")) {
			printf("HQ9COMP: An HQ9+ compiler/interpreter\n");
			printf("Version 1.0\n\n");
			printf("Usage: HQ9COMP [OPTION] SOURCEFILE\n\n");
			printf("Options:\n");
			printf("-?   --help : Display help\n");
			printf("-i --ignore : Ignore syntax errors\n");
			exit(0);
		} else if (strcmp(argv[i],"-i")||strcmp(argv[i],"--ignore")) {
			ignore = true;
		}
	}
	char *cd = getenv("CD");
	memset(fname,0,4096);
	FILE f;
	if (argc>1) {
		if (argv[argc-1][1]==':')
			strcpy(fname,argv[argc-1]);
		else {
			memcpy(fname,cd,strlen(cd));
			memcpy(fname+strlen(cd),argv[argc-1],strlen(argv[argc-1]));
		}
	} else {
		printf("Requires args.\n");
		exit(1);
	}
	f = fopen(fname,"r");
	if (!f.valid) {
		printf("Could not read file (ERR 1).\n");
		exit(1);
	}
	memset(buf,0,513);
	if (fread(&f,buf,0,512)) {
		printf("Could not read file (ERR 2).\n");
		exit(2);
	}
	for (uint16_t i = 0; i < 512; i++) {
		if (buf[i]=='H')
			printf("Hello World\n");
		else if (buf[i]=='Q')
			printf("%s",buf);
		else if (buf[i]=='9') {
			for (int j = 99; j > 1; j--) {
				printf("%d bottles of beer on the wall, %d bottles of beer.\n",j,j);
				printf("Take one down, pass it around. %d bottles of beer on the wall.\n\n",j-1);
			}
			printf("1 bottle of beer on the wall, 1 bottle of beer.\n");
			printf("Take one down, pass it around. No bottles of beer on the wall.\n");
		} else if (buf[i]!='+'&&buf[i]!=' '&&buf[i]!='\n') {
			if (!buf[i])
				exit(0);
			if (!ignore) {
				printf("Syntax error in %d (ERR 127)\n",i);
				exit(127);
			}
		}
	}
	exit(0);
}
