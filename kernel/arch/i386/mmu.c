#include <kernel/mmu.h>

//Based off of http://www.brokenthorn.com/Resources/OSDev17.html "brokenthorn"

#define PHYSICAL_BLOCK_SIZE	4096

static uint32_t phys_memory_size;
static uint32_t* phys_memory_map;
static uint32_t phys_used_blocks;
static uint32_t phys_max_blocks;

void init_physical_memory(uint32_t total_mem) {
	phys_memory_size = total_mem;
	phys_memory_map = (uint32_t*) (0x400000-(32*PHYSICAL_BLOCK_SIZE)); //Start pages just after malloc
	phys_max_blocks = phys_memory_size;
	phys_used_blocks = phys_max_blocks;
	
	memset(phys_memory_map,0xf,phys_used_blocks/8); //One bit per block, set all used
}

void init_mmu_paging(uint32_t total_mem) {
	
}