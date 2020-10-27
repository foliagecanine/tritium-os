#include <stdio.h>
#include <kernel/ksetup.h>
#include <kernel/tty.h>
#include <kernel/multiboot.h>
#include <kernel/pci.h>
#include <kernel/mem.h>
#include <fs/disk.h>
#include <fs/file.h>
#include <fs/fat12.h>
#include <fs/fat16.h>
#include <kernel/syscalls.h>

multiboot_memory_map_t *mmap;
multiboot_info_t *mbi;

void kernel_main(uint32_t magic, uint32_t ebx) {
	serial_init();
	terminal_initialize();
	disable_cursor();
	if (magic!=0x2BADB002) {
		kerror("ERROR: Multiboot info failed! Bad magic value:"); 
		printf("%lx\n",(uint64_t)magic);
		abort();
	}
	printf("Hello, kernel World!\n");
	kprint("[INIT] Mapped memory");
	init_gdt();
	init_idt();
	init_mouse();
	init_pit(10000);
	mbi = (multiboot_info_t *)ebx;
	init_paging(mbi);
	set_current_heap(heap_create(32));
	init_ahci();
	init_file();
	init_syscalls();
	install_tss();
	init_tasking(1);
	init_usb();
	kprint("[KMSG] Kernel initialized successfully");
	
	//Support up to 8 drives (for now)
	for (uint8_t i = 0; i < 8; i++) {
		if (!mountDrive(i)) {
			printf("Mounted drive %u\n",i);
		} else if (i==0) {
			printf("No valid drive found.\n");
			printf("Press shift key to enter Kernel Debug Console.\n");
			for (;;) {
				sleep(1);
				int k = getkey();
				
				if (k==42||k==54||k==170||k==182) {
					printf("KEY DETECTED - INITIALIZING DEBUG CONSOLE...\n");
					debug_console();
				}
			}
		}
	}
	
	if (strcmp(getDiskMount(0).type,"FAT12"))
		FAT12_print_folder(((FAT12_MOUNT *)getDiskMount(0).mount)->RootDirectoryOffset*512+1,32,0);
	
	if (strcmp(getDiskMount(0).type,"FAT16"))
		FAT16_print_folder(((FAT16_MOUNT *)getDiskMount(0).mount)->RootDirectoryOffset*512+1,32,0);
	
	printf("Press shift key to enter Kernel Debug Console.\n");
	for (uint16_t i = 0; i < 1000; i++) {
		sleep(1);
		int k = getkey();
		
		if (k==42||k==54||k==170||k==182) {
			printf("KEY DETECTED - INITIALIZING DEBUG CONSOLE...\n");
			debug_console();
		}
	}
		
	start_program("A:/bin/GUI.SYS");
	//start_program("A:/bin/MEM.PRG");
	//Idle program to prevent errors if the program above exits without any active children.
	FILE prgm = fopen("A:/bin/IDLE.SYS","r");
	if (prgm.valid) {
		void *buf = alloc_page((prgm.size/4096)+1);
		fread(&prgm,buf,0,prgm.size);
		create_process(buf,prgm.size);
	}

	kerror("Kernel has reached end of code.");
}