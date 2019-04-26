#include <kernel/mmu.h>

/*This code is based off of https://github.com/levex/osdev/blob/master/memory/malloc.c
 * but because of a lack of a license, I wrote one myself
 * My functions are a lot more efficient:
 *
 * During allocation, the one above had the space reallocated as so (A = Alloc_entry, U = Used, F = Free, brackets are the memory ACTUALL in use)-
 * AAAA[UUUUUUUUUUUUUUUU] -> AAAAFFFFFFFFFFFFFFFF -> AAAA[UUUU]UUUUUUUUUUUU
 * It marks all of it as used even though it isn't. Eventually this overflows and gives an "Out of Memory" error.
 *
 * My version does as so -
 * AAAA[UUUUUUUUUUUUUUUU] -> AAAAFFFFFFFFFFFFFFFF -> AAAA[UUUU]AAAAFFFFFFFF
 * It just makes a new allocation entry (if there's room, otherwise it just marks as used)
 *
 * Then my free reclaims near blocks - 
 * AAAAFFFFAAAA[UUUU]AAAAFFFF -> AAAAFFFFAAAAFFFFAAAAFFFF -> AAAAFFFFFFFFFFFFFFFFFFFF
 */

char *strcpy(char *dest, const char *src)
{
   char *ptr = dest;
   while(*dest++ = *src++);
   return ptr;
}

typedef struct {
	_Bool status;
	uint32_t size;
} __attribute__ ((packed)) alloc_t;

uint32_t last_alloc_loc, heap_end, heap_start, pheap_start, pheap_end, memory_used = 0;
uint8_t *pheap_descr = 0;

void* memset (void * ptr, int value, size_t num )
{
	unsigned char* p=ptr;
	while(num--)
		*p++ = (unsigned char)value;
	return ptr;
}

void* malloc(size_t size);

#define MALLOC_MAX_PAGES 32

void mmu_info() {
	printf("Memory used: %d\n", (uint64_t)memory_used);
	uint32_t mem_free = heap_end - heap_start - memory_used;
	printf("Memory free: %d", (uint64_t)mem_free);
	printf(" (approx. %d KiB)\n", (uint64_t)(mem_free/1024));
}

void mmu_init(uint32_t krnend) {
	memory_used = 0;
	last_alloc_loc = krnend + 0x1000;
	heap_start = last_alloc_loc;
	pheap_end = 0x400000;
	pheap_start = pheap_end - (MALLOC_MAX_PAGES * 4096);
	heap_end = pheap_start;
	memset((char*)heap_start, 0, heap_end-heap_start);
	pheap_descr = (uint8_t *)malloc(MALLOC_MAX_PAGES);
	kprint("Set up memory allocation. Details listed below:");
	mmu_info();
}


void* malloc(size_t size) {
	_Bool allocate_failed = false;
	
	if (size==0) return 0; //No size? Why allocate?
	
	uint8_t *mem = (uint8_t *)heap_start; //Searching memory
	while ((uint32_t)mem < last_alloc_loc) { //Keep going until we've reached the last allocation point
		alloc_t *alloc = (alloc_t *)mem; //Allocation entry
		
		if (alloc->size==0) { //Missing allocation...
			allocate_failed = true; //Fail flag
			break; //Exit
		}
		
		if (alloc->status) { //If this space is used
			mem+=alloc->size+sizeof(alloc_t); //Skip this
			
		} else { //It's not used
			
			if (alloc->size >= size) { //It's big enough
				alloc->status = true; //Mark as used
				
				if (alloc->size==size||!(size+sizeof(alloc_t)+1>alloc->size)) { //Either perfect size (no wasted space) or not big enough to fit 2 allocs in.
					memset(mem+sizeof(alloc_t), 0, size); //Initialize it with 0's (optional)
					memory_used += size + sizeof(alloc_t); //Record memory as used
					return (char *)(mem + sizeof(alloc_t)); //Give them the location of the actual data, not the allocation
				
				} else { //Too big, but there's enough space to make a new allocation.
					//Find where we'll put our extra alloc
					uint8_t *extra_alloc_loc = mem; 
					extra_alloc_loc+=size+sizeof(alloc_t);
					
					//Make this data into a new alloc.
					alloc_t *extra_alloc = (alloc_t *)extra_alloc_loc;
					extra_alloc->status = false;
					extra_alloc->size = (alloc->size-size)-sizeof(alloc_t);
					
					//We do this at the end to make sure we dont change the location of the extra_alloc
					memory_used+=size; //The other alloc is unused, so don't add that one
					alloc->size = size; 
					return (char *)(mem+sizeof(alloc_t));
				}
				
			} else {
				mem += alloc->size+sizeof(alloc_t);
			}
	}
	}
	
	if (last_alloc_loc+size+sizeof(alloc_t)>= heap_end)
		kerror("Error: Out of Memory!");
	else {
		if (!allocate_failed) {
			//Create a new alloc at the end (we've got enough memory)
			alloc_t *alloc = (alloc_t *)last_alloc_loc;
			alloc->status = true;
			alloc->size = size;
			
			last_alloc_loc+=size+sizeof(alloc_t);
			memory_used += size+sizeof(alloc_t);
			memset((char *)((uint32_t)alloc + sizeof(alloc_t)), 0, size);
			return (char *)((uint32_t)alloc + sizeof(alloc_t));
		} else {
			kerror("Unknown memory allocation error.");
		}
	}
}

void free(void *__ptr) {
	alloc_t *ptr_alloc = (__ptr - sizeof(alloc_t));
	memory_used -= ptr_alloc->size + sizeof(alloc_t);
	ptr_alloc->status = 0;

	uint8_t *mem = heap_start;
	
	//Free upwards
	alloc_t *root_alloc = (alloc_t *)mem; 
	//Start with the first free allocation
	while (mem < last_alloc_loc) {
		if (root_alloc->status) {
			mem+=sizeof(alloc_t)+root_alloc->size;
			root_alloc = (alloc_t *)mem;
		} else {
			break;
		}
	}
	while (mem < last_alloc_loc) {
		mem+=root_alloc->size+sizeof(alloc_t); //Find the next one in the list
		alloc_t *alloc = (alloc_t *)mem;
		
		if (!alloc->status) { //Make sure its free
			root_alloc->size+=alloc->size+sizeof(alloc_t); //Remember we're deleting the ENTIRE entry. This includes the alloc
			mem+=alloc->size+sizeof(alloc_t); //Now see if we can absorb the next one
		} else { //Otherwise the next free one after that becomes the new root_alloc
			while(mem<last_alloc_loc) {
				alloc = (alloc_t *)mem;
				mem+=alloc->size+sizeof(alloc_t);
				if (!alloc->status) { //If this one is free
					root_alloc = (alloc_t *)mem; //Make it the new root_alloc
					break;
				}
			}
		}
	}
	
	
	/*alloc_t *alloc = (alloc_t *)mem;
	mem+=alloc->size+sizeof(alloc_t);
	
	//Free upwards
	while(mem<last_alloc_loc) {
		alloc = (alloc_t *)mem;
		if (!alloc->status) {
			ptr_alloc->size += alloc->size+sizeof(alloc_t);
			mem+=alloc->size+sizeof(alloc_t);
		} else {
			break;
		}
	}*/
}