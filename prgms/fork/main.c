#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
	terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY,VGA_COLOR_BLACK));
	uint32_t newprg = fork();
	uint32_t retval;
	if (newprg) {
		printf("[%$] Hello world! I created %$!\n",getpid(),newprg);
		retval = waitpid(newprg);
		printf("[%$] Child with PID %$ exited\n",getpid(),retval);
	}
	newprg = fork();
	if (newprg) {
		printf("[%$] Hello world! I created %$!\n",getpid(),newprg);
		retval = waitpid(newprg);
		printf("[%$] Child with PID %$ exited\n",getpid(),retval);
	}
	newprg = fork();
	if (newprg) {
		printf("[%$] Hello world! I created %$!\n",getpid(),newprg);
		retval = waitpid(newprg);
		printf("[%$] Child with PID %$ exited\n",getpid(),retval);
	} else
		printf("Hello world! I am a child process with a PID of %$!\n",getpid());
	exit(getpid());
}
