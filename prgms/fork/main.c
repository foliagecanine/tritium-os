#include <stdio.h>
#include <string.h>

void main(uint32_t argc, char **argv) {
	terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY,VGA_COLOR_BLACK));
	uint32_t newprg = fork();
	if (newprg)
		waitpid(newprg);
	newprg = fork();
	if (newprg)
		printf("Hello world! I created %$!\n",newprg);
	else
		printf("Hello world! I am a child process!\n");
	exit(0);
}
