#ifndef KERNEL_MEM_H
#define KERNEL_MEM_H

#include <kernel/stdio.h>

typedef struct {
	uint32_t heap_start;
	uint32_t heap_end;
	uint32_t last_alloc_loc;
	uint32_t heap_size;
} kheap_t;

void identity_map(void *addr);
void map_addr(void *vaddr, void *paddr);
void unmap_vaddr(void *vaddr);
void trade_vaddr(void *vaddr);
void *alloc_page(size_t pages);
void free_page(void *start, size_t pages);
void *calloc_page(size_t pages);
void *alloc_sequential(size_t pages);
void *get_phys_addr(void *vaddr);
void *map_page_to(void *vaddr);
void map_page_secretly(void *vaddr, void *paddr);
void unmap_secret_page(void *vaddr, size_t pages);
void *map_paddr(void *paddr, size_t pages);
void *realloc_page(void *ptr, uint32_t old_pages, uint32_t new_pages);
void mark_user(void *vaddr,_Bool user);
void mark_write(void *vaddr,_Bool write);
kheap_t heap_create(uint32_t pages);
void heap_free(kheap_t heap);
void set_current_heap(kheap_t heap);
kheap_t get_current_heap();
void *malloc(size_t size);
void free(void *__ptr);

#endif
