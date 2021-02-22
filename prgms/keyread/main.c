#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
	terminal_setcolor(0x07);
	printf("Release Esc (code 0x81) to exit\n");
	printf("0x00\n");
	uint8_t g = 0;
	getkey();
	while (g!=0x81) {
		g = getkey();
		if (g)
			printf("0x%#\n",(uint64_t)g);
	}
}
