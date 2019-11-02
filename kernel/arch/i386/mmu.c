#include <kernel/mmu.h>

//Based off of http://www.brokenthorn.com/Resources/OSDev17.html "brokenthorn"
//Also slightly based off of http://www.jamesmolloy.co.uk/tutorial_html/6.-Paging.html to verify brokenthorn

#define PHYSICAL_BLOCK_SIZE	4096

static uint32_t phys_memory_size;
static uint32_t* phys_memory_map;
static uint32_t phys_used_blocks;
static uint32_t phys_max_blocks;

void mark_block(uint32_t addr, size_t size, _Bool used) {
	uint32_t currentBit = addr/PHYSICAL_BLOCK_SIZE-1;
	//If dividend is less than divisor, it returns 0. Make sure it returns 1 if there is 1 byte needed
	for (uint32_t numBlocks = (size+(PHYSICAL_BLOCK_SIZE-1))/PHYSICAL_BLOCK_SIZE; numBlocks>0; numBlocks--) {
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

void init_physical_memory(uint32_t total_mem, multiboot_memory_map_t *mmap, uint32_t krnend) {
	phys_memory_size = total_mem;
	phys_memory_map = (uint32_t*) (0x400000-(32*PHYSICAL_BLOCK_SIZE)); //Start paging just after malloc
	phys_max_blocks = phys_memory_size/PHYSICAL_BLOCK_SIZE;
	phys_used_blocks = phys_max_blocks;
	
	memset(phys_memory_map,0xff,phys_used_blocks/8); //One bit per block, set all used
	
	for (uint8_t i = 0; i < 15; i++) {
		if (i>0&&mmap[i].addr==0)
			break;
		if (mmap[i].type==1) {
			mark_block(mmap[i].addr,mmap[i].len,false);
		}
	}
	//Mark kernel as used
	mark_block(0,krnend,true);
}

static _Bool out_of_memory = false;

uint32_t find_free_blocks(uint16_t amt) {
	out_of_memory = false;
	uint16_t cnt = amt;
	uint32_t ret = 0;
	for (uint32_t i = 0; i < phys_max_blocks/32; i++) {
		if (phys_memory_map[i]!=0xFFFFFFFF) { //If it's all used we can skip it
			for (uint8_t j = 0; j < 32; j++) {
				if (!((phys_memory_map[i]>>(31-j))&1)) {
					if (cnt==amt)
						ret = (i*32)+j;
					cnt--;
					if (cnt==0)
						return ret;
				} else {
					cnt=amt;
				}
			}
		}
	}
	out_of_memory = true;
	return 0;
}

void *alloc_physical_blocks(uint16_t amt) {
	uint32_t return_block = find_free_blocks(amt)*PHYSICAL_BLOCK_SIZE;
	if (!out_of_memory) {
		for (uint16_t i = 0; i < amt; i++) {
			mark_block(return_block+(i*PHYSICAL_BLOCK_SIZE),1,true);
		}
		return (void *)return_block;
	} else {
		return 0;
	}
}

void free_physical_blocks(void *ptr, uint16_t amt) {
	for (uint16_t i = 0; i < amt; i++) {
		mark_block((uint32_t)ptr+(i*PHYSICAL_BLOCK_SIZE),1,false);
	}
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
} page_t;

typedef struct {
	page_t entries[1024];
} page_table_t;

typedef struct {
	page_t entries[1024];
} page_directory_t;

_Bool alloc_virtual_page(page_t *page) {
	uint32_t pageloc = (uint32_t)alloc_physical_blocks(1);
	if (!pageloc)
		return false;
	
	page->frame = pageloc>>12; //Isolate top 20 bits (i.e instead of 0x12345678 -> 0x45678, it does 0x12345678 -> 0x12345)
	page->present = 1;
	
	return true;
}

void free_virtual_page(page_t *page) {
	void *pageloc = (void *)(page->frame<<12);
	if (pageloc)
		free_physical_blocks(pageloc,1);
	page->present = 0;
}

void table_identity_map(page_table_t * identity_map, uint32_t start, _Bool writable, _Bool user) {
	//Fill all 4 MiB entire table (0x0-0x3FFFFFF)
	for (uint32_t i  = 0; i < 0x400000; i+=PHYSICAL_BLOCK_SIZE) {
		page_t new_page;
		memset(&new_page,0,sizeof(page_t));
		
		new_page.present = 1;
		new_page.writable = (writable ? 1 : 0);
		new_page.frame = (i+start)>>12;
		new_page.user = (user ? 1 : 0);
		
		identity_map->entries[i/PHYSICAL_BLOCK_SIZE] = new_page;
	}
}

void init_mmu_paging(uint32_t total_mem, multiboot_memory_map_t *mmap, uint32_t krnend) {
	init_physical_memory(total_mem,mmap,krnend);
	
	page_directory_t *pagedir = alloc_physical_blocks(1);
	memset(pagedir,0,sizeof(page_directory_t));
	
	page_table_t *identity_map = alloc_physical_blocks(1);
	memset(identity_map,0,sizeof(page_table_t));
	page_table_t *user_identity_map = alloc_physical_blocks(1);
	memset(user_identity_map,0,sizeof(page_table_t));
	
	//If our kernel is bigger than 4 MiB then we have a problem
	table_identity_map(identity_map, 0,false, true);
	table_identity_map(user_identity_map,0x400000,true, true);
	
	pagedir->entries[0].present = 1;
	pagedir->entries[0].writable = 0;
	pagedir->entries[0].user = 1;
	pagedir->entries[0].frame = (uint32_t)(uint32_t *)identity_map>>12;
	
	//Create initial user page table
	pagedir->entries[1].present = 1;
	pagedir->entries[1].writable = 1;
	pagedir->entries[1].user = 1;
	pagedir->entries[1].frame = (uint32_t)(uint32_t *)user_identity_map>>12;
	
	asm("mov %0, %%cr3; mov %%cr0, %%eax; or $0x80000000, %%eax; mov %%eax,%%cr0" : : "r"(pagedir));
	
	kprint("Paging enabled.");
}