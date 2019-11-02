#include <kernel/stdio.h> //Common functions: printf, getchar, etc.

#include <kernel/tty.h> //Extra terminal functions: terminal_initialize, terminal_clear, etc.
#include <kernel/init.h> //Mostly unused. Initialization functions.
#include <kernel/debug.h> //Debug console: debug_console
#include <kernel/mmu.h> //Memory management: malloc, init_mmu_paging, etc.
#include <kernel/multiboot.h> //Multiboot standard header. Includes things like the memory map layout
#include <kernel/idt.h> //Interrupt descriptor table: init_idt
#include <kernel/exceptions.h> //Exceptions
#include <fs/fs.h> //Filesystems: 
#include <fs/disk.h> //ATA Driver: init_ata
#include <fs/fat12.h> //FAT12 driver: detect_fat12
#include <fs/file.h> //The file management driver
#include <kernel/syscalls.h> //Syscalls: init_syscalls
#include <kernel/pci.h> //TEMP: REMOVE LATER

extern uint32_t krnend;

uint32_t himem = 0;
uint32_t lomem = 0;
multiboot_memory_map_t *mmap;
multiboot_info_t *mbi;

void init() {
	initialize_gdt(); //Total number of bytes so our GDT doesn't fail
	mmu_init(&krnend);
	init_idt();
	init_pit(1000);
	init_mmu_paging((lomem + himem)*1024,mmap,&krnend);
	
	init_ata();
}

const char * mem_types[] = {
	"ERROR",
	"Available",
	"Reserved",
	"ACPI Reclaimable",
	"NVS",
	"Bad Ram"
};

void print_memory_layout() {
	for (uint8_t i = 0; i < 15; i++) {
		uint32_t type = mmap[i].type;
		if (mmap[i].type>5)
			type = 2;
		if (i>0&&mmap[i].addr==0)
			break;
		printf("%d: Start=0x%#, Length=0x%# bytes, type=%d (%s)\n",(uint32_t)i,(uint64_t)mmap[i].addr,(uint64_t)mmap[i].len,(uint32_t)type,mem_types[type]);
	}
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
	mbi = (multiboot_info_t *)ebx;
	mmap = (multiboot_memory_map_t *)mbi->mmap_addr;
	
	
	terminal_initialize();
	disable_cursor();
	printf("Hello World!\n");
	printf("Kernel Start: 0x%#, Kernel End: 0x%#\n", (uint64_t)0x1000, (uint64_t)&krnend);
	
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
		
		printf("Total memory: %dGB (%dMB)\n",(uint32_t)totalMem/1048576,(uint32_t)totalMem/1024);
		#endif
	}
	
	print_memory_layout();
	
	init();
	
	//Install syscalls before we enter usermode
	init_syscalls();
	
	//Try mounting the first four disks
	for (uint8_t i = 0; i < 4; i++)
		mountDrive(i);
	
	printf("Press the shift key to enter debug mode.\n");
	//Give some time to read and respond
	
	for (uint16_t i = 0; i < 1000; i++) {
		sleep(1);
		int k = getkey();
		
		if (k==42||k==54||k==170||k==182) {
			printf("KEY DETECTED - INITIALIZING DEBUG CONSOLE...\n");
			debug_console();
		}
	}
	
	printf("Loading TritiumOS.\n");
	
	sleep(1000);
	
	terminal_clear();

	printf("Loading TritiumOS.\n");
	
	int count = 1;
	for (uint16_t i = 0; i < 256; i++) {
		for (uint8_t j = 0; j < 32; j++) {
			for (uint8_t k = 0; k < 8; k++) {
				pci_t pci = getPCIData(i,j,k);
				if (pci.vendorID!=0xFFFF) {
					printf("%#:%#.%d [%#:%#] (%#.%#) 0:%# 1:%# 2:%# 3:%# 4:%# 5:%#\n",(uint64_t)i,(uint64_t)j,(uint32_t)k,(uint64_t)pci.vendorID,(uint64_t)pci.deviceID,(uint64_t)pci.classCode,(uint64_t)pci.subclass,(uint64_t)pci.BAR0,(uint64_t)pci.BAR1,(uint64_t)pci.BAR2,(uint64_t)pci.BAR3,(uint64_t)pci.BAR4,(uint64_t)pci.BAR5);
				}
			}
		}
	}
		
	while(true) {
		
	}
	
	//End of kernel. Add any extra info you need for debugging in the line below.
	printf("Extra info: %d\n", 0);
	kerror("Kernel has reached end of kernel_main. Is this intentional?");
	
	while (1)
		;
}
