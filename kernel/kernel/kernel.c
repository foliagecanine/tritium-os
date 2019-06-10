#include <kernel/stdio.h>

#include <kernel/tty.h>
#include <kernel/init.h>
#include <kernel/mmu.h>
#include <kernel/multiboot.h>
#include <kernel/idt.h>
#include <kernel/exceptions.h>
#include <fs/fs.h>
#include <fs/disk.h>
#include <fs/fat12.h>
#include <fs/file.h>

extern uint32_t krnend;

void init() {
	initialize_gdt();
	mmu_init(&krnend);
	init_idt();
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

uint32_t total_mem = 0;

void kernel_main(multiboot_info_t* mbi, unsigned int magic) {
	//Set total_mem to right value
	if (mbi->flags & 0b01000000) {
		total_mem = mbi->mem_upper+mbi->mem_lower;
	}
	
	terminal_initialize();
	disable_cursor();
	printf("Hello Kernel World!\n");
	printf("Total memory: %d | Magic: %d\n",(uint64_t)total_mem, magic);
	init();
	if (!drive_exists(0))
		printf("No drive in drive 0.\n");
	if (detect_fat12(0)) {
		printf("Drive 0 is formatted FAT12\n");
	} else {
		printf("Drive 0 is NOT formatted FAT12\n");
	}
	
	printf("Attempting to mount drive 0.\n");
	uint8_t mntErr = mountDrive(0);
	if (!mntErr) {
		printf("Successfully mounted drive!\n");
	} else {
		printf("Mount error %d\n",(uint64_t)mntErr);
	}
	
	//Test invalid opcode
	/*int a;
	a = 1/0;
	printf("%d\n",a);*/
	
	//Test division by zero
	/*asm volatile(	
	"mov $0, %edx\n"
	"mov $0, %eax\n"
	"mov $0, %ecx\n"
	"div %ecx"
	);*/
	
	char fame[12];
	strcpy(fame,"A:/testfldr");
	FILE myfile = fopen(fame,"r+");
	printf("Size of A:/testfldr is %d\n",myfile.size);
	
	enter_usermode();
	
	//End of kernel. Add any extra info you need for debugging in the line below.
	printf("Extra info: %d\n", 0);
	kerror("Kernel has reached end of kernel_main. Is this intentional?");
	/*for (;;) {
		char c = getchar();
		if (c) {
			printf("%c",c);
		} else if (getkey()==14) {
			terminal_backup();
			putchar(' ');
			terminal_backup();
		}
	}*/
	while (1)
		;
}
