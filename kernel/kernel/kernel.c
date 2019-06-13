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
#include <kernel/syscalls.h>

extern uint32_t krnend;

uint32_t himem = 0;
uint32_t lomem = 0;

void init() {
	initialize_gdt((lomem + himem)*1024); //Total number of bytes so our GDT doesn't fail
	mmu_init(&krnend);
	init_idt();
	init_pit(1000);
	init_mmu_paging();
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

void kernel_main(uint32_t magic, uint32_t ebx) {
	
	//Check if multiboot worked
	if (magic!=0x2BADB002) {
		terminal_initialize();
		disable_cursor();
		kerror("ERROR: Multiboot info failed! Bad magic value below:");
		printf("%#\n",(uint64_t)magic);
		abort();
	}
	
	//Retreive multiboot info
	multiboot_info_t *mbi = (multiboot_info_t *)ebx;
	
	terminal_initialize();
	disable_cursor();
	printf("Hello World!\n");
	
	//Process multiboot memory
	if (mbi->flags & MULTIBOOT_INFO_MEMORY) {
		lomem = (uint32_t)mbi->mem_lower;
		himem = (uint32_t)mbi->mem_upper;
		
		#ifdef DEBUG
		printf("lomem = %dKB, himem = %dKB\n", lomem, himem);
		#else
		
		//1048576 is amount of kibibytes in a gibibyte. Add a mebibyte because it doesn't count the first mebibyte
		//post scriptum, kibi/mebi/gibi vs kilo/mega/giga is really annoying
		uint64_t totalMem = ((uint64_t)lomem+(uint64_t)himem+1024);
		
		//Two separate printf's because inline math only works once for some reason
		printf("Total memory: %dGB ",totalMem/1048576);
		printf("(%dMB)\n",totalMem/1024);
		#endif
	}
	
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
	
	//Install syscalls before we enter usermode
	init_syscalls();
	
	kprint("Entering usermode! Hold tight!");
	enter_usermode();
	//We're in usermode (hopefully)! Lets see what we can do
	
	printf("It looks like we never left Kansas.\n");
	//Test syscalls
	char * teststring = "Hello Syscall World!\n";
	asm volatile("lea (%1),%%ebx; int $0x80" : : "a" (0), "r" ((int)teststring));
	
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
