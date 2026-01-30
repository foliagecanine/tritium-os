#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
	terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
	uint32_t newprg = fork();
	uint32_t retval;
	if (newprg) {
		printf("[%u] Hello world! I created %u!\n", getpid(), newprg);
		retval = waitpid(newprg);
		printf("[%u] Child with PID %u exited\n", getpid(), retval);
	}
	newprg = fork();
	if (newprg) {
		printf("[%u] Hello world! I created %u!\n", getpid(), newprg);
		retval = waitpid(newprg);
		printf("[%u] Child with PID %u exited\n", getpid(), retval);
	}
	newprg = fork();
	if (newprg) {
		printf("[%u] Hello world! I created %u!\n", getpid(), newprg);
		retval = waitpid(newprg);
		printf("[%u] Child with PID %u exited\n", getpid(), retval);
	} else
		printf("Hello world! I am a child process with a PID of %u!\n", getpid());
	exit(getpid());
}
