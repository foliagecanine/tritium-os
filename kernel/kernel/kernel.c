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

void display_sector_data(uint8_t disk, uint32_t sector, uint16_t amt) {
	if (!drive_exists(disk))
		return;
	uint8_t read[512];
	read_sector_lba(disk,sector,read);
	printf("Data: ");
	for (int i = 0; i<amt; i++) {
		if (read[i]<16) {
			printf("0");
		}
		printf("%# ", (uint64_t)(read[i]));
	}
	printf("\n");
}

void kernel_main(void) {
	
	terminal_initialize();
	disable_cursor();
	printf("Hello Kernel World!\n");
	init();
	if (drive_exists(0))
		printf("First 32 bytes of drive 0:\n");
	else
		printf("No drive in drive 0, so not printing\n");
	display_sector_data(0,0,32);
	if (detect_fat12(0)) {
		printf("Drive 0 is formatted FAT12\n");
	} else {
		printf("Drive 0 is NOT formatted FAT12\n");
	}
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
