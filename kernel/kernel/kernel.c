#include <kernel/stdio.h>

#include <kernel/tty.h>
#include <kernel/init.h>
#include <kernel/mmu.h>
#include <kernel/idt.h>
#include <kernel/fs.h> //Temporary, replace with file.h later (once fat12 & files implemented)

extern uint32_t krnend;

void init() {
	initialize_gdt();
	mmu_init(&krnend);
	init_idt();
	init_kbd();
	init_ata();
}

void kernel_main(void) {
	
	terminal_initialize();
	disable_cursor();
	printf("Hello Kernel World!\n");
	init();
	uint8_t disk[512];
	printf("Status: %d\n", (uint64_t)read_sectors_lba(0,0,1,disk));
	printf("Data: ");
	for (int i = 0; i<256; i++) {
		if (disk[i]<16) {
			printf("0");
		}
		printf("%# ", (uint64_t)(disk[i]));
	}
	printf("\n");
	printf("Extra info: %d\n", 0);
	kerror("Kernel has reached end of kernel_main. Is this intentional?");
	for (;;) {
		char c = getchar();
		if (c) {
			printf("%c",c);
		} else if (getkey()==14) {
			terminal_backup();
			putchar(' ');
			terminal_backup();
		}
	}
}
