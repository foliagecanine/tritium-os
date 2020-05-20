#include <stdio.h>
#include <kernel/ksetup.h>
#include <kernel/tty.h>
#include <kernel/multiboot.h>
#include <kernel/pci.h>
#include <fs/disk.h>
#include <fs/file.h>
#include <kernel/syscalls.h>

multiboot_memory_map_t *mmap;
multiboot_info_t *mbi;

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
	mbi = (multiboot_info_t *)ebx;
	init_paging(mbi);
	init_ahci();
	init_file();
	init_syscalls();
	kprint("[KMSG] Kernel initialized successfully");
	
	sleep(1000);
	
	if (!mountDrive(0)) {
		printf("Mounted drive 0\n");
	} else {
		printf("No valid drive found.\n");
		for(;;);
	}
	
	FILE txt = fopen("A:/TESTPRG.PRG","r");
	char *read = map_page_to((void *)0x100000);
	memset(read,0,4096);
	fread(&txt,read,0,txt.size);
	
	asm("jmp 0x100000");
}