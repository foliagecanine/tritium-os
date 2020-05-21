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

//This is the code to switch to usermode. We'll put it in the page directly before 0x100000
unsigned char enter_usermode[] = {
  0xfa, 0x66, 0xb8, 0x23, 0x00, 0x8e, 0xd8, 0x8e, 0xc0, 0x8e, 0xe0, 0x8e,
  0xe8, 0x6a, 0x23, 0x54, 0x9c, 0x6a, 0x1b, 0x68, 0x00, 0x00, 0x10, 0x00,
  0xcf, 0x90, 0x90, 0x90
};
unsigned int eu_len = 28;

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
	install_tss();
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
	mark_user((void *)0x100000,true);
	memset(read,0,4096);
	fread(&txt,read,0,txt.size);
	
	//Usermode time!
	asm volatile("\
		cli; \
		mov $0x23, %ax; \
		mov %ax, %ds; \
		mov %ax, %es; \
		mov %ax, %fs; \
		mov %ax, %gs; \
		push $0x23; \
		push %esp; \
		pushf; \
		push $0x1B; \
		push $0x100000; \
		iret; \
	");
}