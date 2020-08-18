#include <kernel/mem.h>
/*
 * My malloc does as so -
 * AAAA[UUUUUUUUUUUUUUUU] -> AAAAFFFFFFFFFFFFFFFF -> AAAA[UUUU]AAAAFFFFFFFF
 * It just makes a new allocation entry (if there's room, otherwise it just marks as used)
 *
 * Then my free reclaims near blocks - 
 * AAAAFFFFAAAA[UUUU]AAAAFFFF -> AAAAFFFFAAAAFFFFAAAAFFFF -> AAAAFFFFFFFFFFFFFFFFFFFF
 */

typedef struct {
	_Bool status;
	uint32_t size;
} __attribute__ ((packed)) alloc_t;

kheap_t current_heap;

kheap_t heap_create(uint32_t pages) {
	kheap_t ret;
	void *mem = alloc_page(pages);
	ret.heap_start = (uint32_t)mem;
	ret.last_alloc_loc = ret.heap_start;
	ret.heap_end = ret.heap_start+(pages*4096);
	ret.heap_size = pages;
	memset((char*)ret.heap_start, 0, ret.heap_end-ret.heap_start);
	return ret;
}

void heap_free(kheap_t heap) {
	free_page((void *)heap.heap_start,heap.heap_size);
}

void set_current_heap(kheap_t heap) {
	current_heap = heap;
}

kheap_t get_current_heap() {
	return current_heap;
}

void* malloc(size_t size) {
	_Bool allocate_failed = false;
	
	if (size==0)
		return 0; //No size? Why allocate?
	
	uint8_t *mem = (uint8_t *)current_heap.heap_start; //Searching memory
	while ((uint32_t)mem < current_heap.last_alloc_loc) { //Keep going until we've reached the last allocation point
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
				
				if (alloc->size==size||size+sizeof(alloc_t)+1<=alloc->size) { //Either perfect size (no wasted space) or not big enough to fit 2 allocs in.
					memset(mem+sizeof(alloc_t), 0, size); //Initialize it with 0's (optional)
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
					alloc->size = size; 
					return (char *)(mem+sizeof(alloc_t));
				}
				
			} else {
				mem += alloc->size+sizeof(alloc_t);
			}
	}
	}
	
	if (current_heap.last_alloc_loc+size+sizeof(alloc_t)>= current_heap.heap_end) {
		kerror("Error: Out of Memory!");
		return 0;
	} else {	
		if (!allocate_failed) {
			//Create a new alloc at the end (we've got enough memory)
			alloc_t *alloc = (alloc_t *)current_heap.last_alloc_loc;
			alloc->status = true;
			alloc->size = size;
			
			current_heap.last_alloc_loc+=size+sizeof(alloc_t);
			memset((char *)((uint32_t)alloc + sizeof(alloc_t)), 0, size);
			return (char *)((uint32_t)alloc + sizeof(alloc_t));
		} else {
			kerror("Unknown memory allocation error.");
			return 0;
		}
	}
}

void free(void *__ptr) {
	alloc_t *ptr_alloc = (__ptr - sizeof(alloc_t));
	ptr_alloc->status = 0;

	uint8_t *mem = (uint8_t *)current_heap.heap_start;
	
	//Free upwards
	volatile alloc_t *root_alloc = (alloc_t *)mem; 
	//Start with the first free allocation
	while ((uint32_t)mem < current_heap.last_alloc_loc) {
		if (root_alloc->status) {
			mem+=sizeof(alloc_t)+root_alloc->size;
			root_alloc = (alloc_t *)mem;
		} else {
			break;
		}
	}
	while ((uint32_t)mem < current_heap.last_alloc_loc) {
		mem+=root_alloc->size+sizeof(alloc_t); //Find the next one in the list
		alloc_t *alloc = (alloc_t *)mem;
		
		if (!alloc->status) { //Make sure its free
			root_alloc->size+=alloc->size+sizeof(alloc_t); //Remember we're deleting the ENTIRE entry. This includes the alloc
			mem+=alloc->size+sizeof(alloc_t); //Now see if we can absorb the next one
		} else { //Otherwise the next free one after that becomes the new root_alloc
			while((uint32_t)mem<current_heap.last_alloc_loc) {
				alloc = (alloc_t *)mem;
				mem+=alloc->size+sizeof(alloc_t);
				if (!alloc->status) { //If this one is free
					root_alloc = (alloc_t *)mem; //Make it the new root_alloc
					break;
				}
			}
		}
	}
	if ((uint32_t)root_alloc+root_alloc->size+sizeof(alloc_t)>=current_heap.last_alloc_loc) {
		current_heap.last_alloc_loc = (uint32_t)root_alloc;
		root_alloc->size = 0;
	}
}