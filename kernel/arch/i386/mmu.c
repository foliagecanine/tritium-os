#include <kernel/mmu.h>

//based off of https://wiki.osdev.org/Setting_Up_Paging and https://wiki.osdev.org/Paging

uint32_t pagedir[1024] __attribute__((aligned(4096)));
uint32_t page_table_1[1024] __attribute__((aligned(4096)));

extern void load_pagedir(unsigned int*);
extern void enablePaging();

void init_mmu_paging() {
	for(int i = 0; i < 1024; i++)
	{
		//Flags: S,RW
		pagedir[i] = 0x00000002;
	}
	 
	//4MiB
	for(unsigned int i = 0; i < 1024; i++)
	{
		page_table_1[i] = (i * 0x1000) | 3;
	}

	//Flags: S,RW,P
	pagedir[0] = ((unsigned int)page_table_1) | 3;
	
	load_pagedir(pagedir);
	enablePaging();
	
	kprint("Paging enabled.");	
}

void * vaddr_to_paddr(void * vaddr) {
	uint32_t pdi = (uint32_t)vaddr >> 22;
    uint32_t pti = (uint32_t)vaddr >> 12 & 0x03FF;
 
    uint32_t * pd = (uint32_t *)0xFFFFF000;
    uint32_t * pt = ((uint32_t *)0xFFC00000) + (0x400 * pdi);
 
    return (void *)((pt[pti] & ~0xFFF) + ((uint32_t)vaddr & 0xFFF));
}

void map_page(void * paddr, void * vaddr, unsigned int flags) {
	uint32_t pdi = (uint32_t)vaddr >> 22;
    uint32_t pti = (uint32_t)vaddr >> 12 & 0x03FF;
 
    uint32_t * pd = (uint32_t *)0xFFFFF000;
    uint32_t * pt = ((uint32_t *)0xFFC00000) + (0x400 * pdi);
	
	pt[pti] = ((uint32_t)paddr) | (flags & 0xFF) | 0x1;
}