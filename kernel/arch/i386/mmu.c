#include <kernel/mmu.h>

//Based off of http://www.brokenthorn.com/Resources/OSDev17.html "brokenthorn"

#define PHYSICAL_BLOCK_SIZE	4096

static uint32_t phys_memory_size;
static uint32_t* phys_memory_map;
static uint32_t phys_used_blocks;
static uint32_t phys_max_blocks;

void mark_block(uint32_t addr, size_t size, _Bool used) {
	uint32_t currentBit = addr/PHYSICAL_BLOCK_SIZE;
	for (uint32_t numBlocks = size/PHYSICAL_BLOCK_SIZE; numBlocks>=0; numBlocks--) {
		currentBit++;
		//32 bits in a uint32_t. Set the bit specified and update other variables.
		if (used) {
			phys_memory_map[currentBit/32] |= 0x80000000>>(currentBit%32);
			phys_used_blocks++; //We marked a block as used. Update used blocks.
		} else {
			phys_memory_map[currentBit/32] &= ~(0x80000000>>(currentBit%32));
			phys_used_blocks--; //We marked a block as unused. Update used blocks.
		}
	}
}

void init_physical_memory(uint32_t total_mem, multiboot_memory_map_t *mmap) {
	phys_memory_size = total_mem;
	phys_memory_map = (uint32_t*) (0x400000-(32*PHYSICAL_BLOCK_SIZE)); //Start paging just after malloc
	phys_max_blocks = phys_memory_size;
	phys_used_blocks = phys_max_blocks;
	
	memset(phys_memory_map,0xf,phys_used_blocks/8); //One bit per block, set all used
	
	for (uint8_t i = 0; i < 15; i++) {
		if (i>0&&mmap[i].addr==0)
			break;
		if (mmap[i].type==1) {
			
		}
	}
}

void init_mmu_paging(uint32_t total_mem) {
	
}