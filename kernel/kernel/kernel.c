#include <kernel/stdio.h>

#include <kernel/tty.h>
#include <kernel/init.h>
#include <kernel/mmu.h>
#include <kernel/idt.h>

extern uint32_t krnend;

void init() {
	initialize_gdt();
	mmu_init(&krnend);
	init_idt();
	init_kbd();
}

void kernel_main(void) {
	
	terminal_initialize();
	disable_cursor();
	printf("Hello Kernel World!\n");
	init();
	printf("Extra info: %d\n", 0);
	kerror("Kernel has reached end of kernel_main. Is this intentional?");
	for(;;) {
		char c = getchar();
		if (c) {
			printf("%c",c);
		} else if (getkey()==14) {
			terminal_backup();
			putchar(' ');
			terminal_backup();
		}
		//int c = getscancode();
		//if (c) {
		//	printf("%d ",c);
		//}
	}
}
