#include <stdio.h>
#include <kernel/ksetup.h>
#include <kernel/tty.h>
#include <kernel/multiboot.h>

multiboot_memory_map_t *mmap;
multiboot_info_t *mbi;

const char * mem_types[] = {"ERROR","Available","Reserved","ACPI Reclaimable","NVS","Bad Ram"};

void kernel_main(uint32_t magic, uint32_t ebx) {
	if (magic!=0x2BADB002) {
		terminal_initialize();
		disable_cursor();
		kerror("ERROR: Multiboot info failed! Bad magic value:"); 
		printf("%#\n",(uint64_t)magic);
		abort();
	}
	
	terminal_initialize();
	disable_cursor();
	printf("Hello, kernel World!\n");
	kprint("[INIT] Mapped memory");
	
	init_gdt();
	init_idt();
	init_pit(1000);
	init_paging();
	kprint("[KMSG] Kernel initialized successfully");
	
	mbi = (multiboot_info_t *)ebx;
	identity_map(mbi);
	//This one should already be mapped. They are in the same page.
	mmap = (multiboot_memory_map_t *)mbi->mmap_addr;
	
	for (uint8_t i = 0; i < 15; i++) {
		uint32_t type = mmap[i].type;
		if (mmap[i].type>5)
			type = 2;
		if (i>0&&mmap[i].addr==0)
			break;
		printf("%d: 0x%#+0x%# %s\n",(uint32_t)i,(uint64_t)mmap[i].addr,(uint64_t)mmap[i].len,mem_types[type]);
	}
}