#include <kernel/stdio.h>

#include <kernel/tty.h>
#include <kernel/init.h>
#include <kernel/kprint.h>

void init() {
	initialize_gdt();
}

void kernel_main(void) {
	terminal_initialize();
	disable_cursor();
	init();
	printf("Hello Kernel World!\n");
	kerror("Kernel has reached end of Main. Is this intentional?");
}
