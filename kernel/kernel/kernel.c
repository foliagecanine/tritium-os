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
multiboot_memory_map_t *mmap;
multiboot_info_t *mbi;

void init() {
	initialize_gdt((lomem + himem)*1024); //Total number of bytes so our GDT doesn't fail
	mmu_init(&krnend);
	init_idt();
	init_pit(1000);
	init_mmu_paging((lomem + himem)*1024,mmap);
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
	
	kprint("Entering usermode! Hold tight!");
	enter_usermode();
	//We're in usermode (hopefully)! Lets see what we can do
	
	printf("It looks like we never left Kansas.\n");
	//Test syscalls
	char * teststring = "Hello Syscall World!\n";
	asm volatile("lea (%1),%%ebx; int $0x80" : : "a" (0), "r" ((uint32_t)teststring));
	
	//End of kernel. Add any extra info you need for debugging in the line below.
	printf("Extra info: %d\n", 0);
	kerror("Kernel has reached end of kernel_main. Is this intentional?");
	
	while (1)
		;
}
