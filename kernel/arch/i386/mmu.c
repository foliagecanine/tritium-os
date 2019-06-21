#include <kernel/mmu.h>

//Based off of http://www.brokenthorn.com/Resources/OSDev17.html "brokenthorn"
//Also slightly based off of http://www.jamesmolloy.co.uk/tutorial_html/6.-Paging.html to verify brokenthorn

#define PHYSICAL_BLOCK_SIZE	4096

static uint32_t phys_memory_size;
static uint32_t* phys_memory_map;
static uint32_t phys_used_blocks;
static uint32_t phys_max_blocks;

void mark_block(uint32_t addr, size_t size, _Bool used) {
	uint32_t currentBit = addr/PHYSICAL_BLOCK_SIZE;
	for (uint32_t numBlocks = size/PHYSICAL_BLOCK_SIZE; numBlocks>0; numBlocks--) {
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
	phys_max_blocks = phys_memory_size/PHYSICAL_BLOCK_SIZE;
	phys_used_blocks = phys_max_blocks;
	
	memset(phys_memory_map,0xf,phys_used_blocks/8); //One bit per block, set all used
	
	for (uint8_t i = 0; i < 15; i++) {
		if (i>0&&mmap[i].addr==0)
			break;
		if (mmap[i].type==1) {
			mark_block(mmap[i].addr,mmap[i].len,false);
		}
	}
}

_Bool out_of_memory = false;

uint32_t find_free_block() {
	for (uint32_t i = 0; i < phys_max_blocks/32; i++) {
		if (phys_memory_map[i]!=0xFFFFFFFF) { //If it's all used we can skip it
			for (uint8_t j = 0; j < 32; j++) {
				if (!((phys_memory_map[i]>>j)&1)) {
					return (i*32)+j;
				}
			}
		}
	}
	out_of_memory = true;
}

void *alloc_physical_block() {
	uint32_t return_block = find_free_block()*PHYSICAL_BLOCK_SIZE;
	if (!out_of_memory) {
		mark_block(return_block,1,true);
		return (void *)return_block;
	} else {
		return 0;
	}
}

void free_physical_block(void *ptr) {
	mark_block(ptr,1,false);
	out_of_memory = false;
}

typedef struct {
   uint32_t present				:1;
   uint32_t writable				:1;
   uint32_t user					:1;
   uint32_t writethrough		:1;
   uint32_t not_cacheable	:1;
   uint32_t accessed			:1;
   uint32_t dirty					:1;
   uint32_t page_alloc_table:1;
   uint32_t global_cpu			:1;
   uint32_t global_lv4			:1;
   uint32_t unused				:2;
   uint32_t frame				:20;
} __attribute__((packed)) page_t;

typedef struct {
	page_t entries[1024];
} __attribute__((packed)) __attribute__((aligned(4096))) page_table_t;

typedef struct {
	uint32_t entries[1024];
} __attribute__((packed)) __attribute__((aligned(4096))) page_directory_t;

_Bool alloc_virtual_page(uint32_t *page_table_entry) {
	void * ptr = alloc_physical_block();
	if (!ptr)
		return false;
	
	((page_t *)page_table_entry)->frame = ptr;
	((page_t *)page_table_entry)->present = 1;
	
	return true;
}

void free_virtual_page(uint32_t *page_table_entry) {
	void* p = (void *)(((page_t *)page_table_entry)->frame);
	if (p)
		free_physical_block(p);
	((page_t *)page_table_entry)->present = 0;
}

uint32_t *find_page_table_entry(page_table_t *page_entry, uint32_t address) {
	if (page_entry)
		return &page_entry->entries[(((address) >> 12) & 0x3ff)];
	return 0;
}

uint32_t *find_page_directory_entry(page_directory_t *page_entry, uint32_t address) {
	if (page_entry)
		return &page_entry->entries[(((address) >> 12) & 0x3ff)];
	return 0;
}

uint32_t *current_page_directory = 0;

inline void set_page_dir(uint32_t *pagedir) {
	asm volatile("mov %0, %%cr3" : : "r"(pagedir) );
}

_Bool load_new_page_directory(uint32_t *page_dir) {
	if (!page_dir)
		return false;
	current_page_directory = page_dir;
	set_page_dir(current_page_directory);
	return true;
}



void init_mmu_paging(uint32_t total_mem, multiboot_memory_map_t *mmap) {
	init_physical_memory(total_mem,mmap);
}