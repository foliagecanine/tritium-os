#include <kernel/ksetup.h>

typedef struct {
	uint8_t present:1;
	uint8_t readwrite:1;
	uint8_t user:1;
	uint8_t writethru:1;
	uint8_t cachedisable:1;
	uint8_t access:1;
	uint8_t zero:1;
	uint8_t size:1;
	uint8_t ignore:4;
	uint32_t address:20;
} __attribute__((packed)) page_dir_entry;

typedef struct {
	uint8_t present:1;
	uint8_t readwrite:1;
	uint8_t user:1;
	uint8_t writethru:1;
	uint8_t cached:1;
	uint8_t access:1;
	uint8_t dirty:1;
	uint8_t zero:1;
	uint8_t ignore:4;
	uint32_t address:20;
} __attribute__((packed)) page_table_entry;

page_dir_entry kernel_page_dir[1024] __attribute__((aligned(4096)));
page_table_entry new_base_page_table[1024] __attribute__((aligned(4096)));
page_table_entry full_kernel_table_storage[1024] __attribute__((aligned(4096)));

page_table_entry *kernel_tables = (page_table_entry *)0xC0400000;

#define pagedir_addr 768

void init_paging() {
	//Stage 1:
	//First, we need to identity map a new portion of RAM so that we have enough space for a new paging table
	//We don't have any programs we're going to break, so any ram around us *should* be fine.
	kernel_page_dir[pagedir_addr].present = 1;
	kernel_page_dir[pagedir_addr].readwrite = 1;
	uint32_t nbpt_addr = (uint32_t)&new_base_page_table[0];
	nbpt_addr -= 0xC0000000;
	nbpt_addr >>=12;
	kernel_page_dir[pagedir_addr].address = nbpt_addr;
	for (uint16_t i = 0; i < 1023; i++) {
		if ((i*4096)>=0x100000) {
			new_base_page_table[i].present = 1;
			new_base_page_table[i].readwrite = 1;
			new_base_page_table[i].address = i;
		}
	}
	new_base_page_table[1023].present = 1;
	new_base_page_table[1023].readwrite = 1;
	new_base_page_table[1023].address = 0x000B8;
	
	uint32_t new_addr = (uint32_t)&kernel_page_dir[0];
	new_addr-=0xC0000000;
	asm volatile("mov %0, %%cr3":: "r"(new_addr));
	
	//Stage 2:
	//Next, we allocate 4MiB of space for our new FULL page table.
	
	kernel_page_dir[pagedir_addr+1].present = 1;
	kernel_page_dir[pagedir_addr+1].readwrite =1;
	kernel_page_dir[pagedir_addr+1].address = (((uint32_t)&full_kernel_table_storage)-0xC0000000)>>12;
	
	for (uint16_t i = 0; i < 1024; i++) {
		full_kernel_table_storage[i].present = 1;
		full_kernel_table_storage[i].readwrite = 1;
		full_kernel_table_storage[i].address = (0x400000>>12)+i;
	}
	
	//Reset the CR3 so it flushes the TLB
	asm volatile("movl %cr3, %ecx; movl %ecx, %cr3");
	
	//Stage 3: 
	//Create a new full page table so we can allocate things whenever we want
	
	//First let's do a memcpy so we dont have any random data
	memset((void*)0xC0400000,0,0x400000);
	
	for (uint16_t i = 0; i < 1023; i++) {
		if ((i*4096)>=0x100000) {
			kernel_tables[(768*1024)+i].present = 1;
			kernel_tables[(768*1024)+i].readwrite = 1;
			kernel_tables[(768*1024)+i].address = i;
		}
	}
	
	kernel_tables[(768*1024)+1023].present = 1;
	kernel_tables[(768*1024)+1023].readwrite = 1;
	kernel_tables[(768*1024)+1023].address = 0x000B8;
	
	for (uint16_t i = 0; i < 1024; i++) {
		kernel_tables[(769*1024)+i].present = 1;
		kernel_tables[(769*1024)+i].readwrite = 1;
		kernel_tables[(769*1024)+i].address = (0x400000>>12)+i;
	}
	
	//Now we link the tables to the page directory, whose location won't change
	for (uint16_t i = 0; i < 1024; i++) {
		kernel_page_dir[i].address = 0x400+(i);
		kernel_page_dir[i].readwrite = 1;
		kernel_page_dir[i].present = 1;
	}
	
	//Reset the CR3 so it flushes the TLB
	asm volatile("movl %cr3, %ecx; movl %ecx, %cr3");
	
	//Awesome! Now the default page tables are at vaddr:0xC0400000 and paddr:0x400000
	//This won't get in the way of programs (which are loaded at vaddr:0x400000) but is still within a low capacity of memory (we only need 8MiB of memory right now)
	kprint("[INIT] Paging initialized");
}

void identity_map(void *addr) {
	uint32_t page = (uint32_t)addr;
	page/=4096;
	volatile uint32_t directory_entry = page/1024;
	volatile uint32_t table_entry = page % 1024;
	kernel_tables[(directory_entry*1024)+table_entry].address = page;
	kernel_tables[(directory_entry*1024)+table_entry].readwrite = 1;
	kernel_tables[(directory_entry*1024)+table_entry].present = 1;
	asm volatile("movl %cr3, %ecx; movl %ecx, %cr3");
}