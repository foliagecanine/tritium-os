#ifndef KERNEL_MEM_H
#define KERNEL_MEM_H

void identity_map(void *addr);
void map_addr(void *vaddr, void *paddr);
void unmap_addr(void *vaddr);
void *alloc_page(size_t pages);
void free_page(void *start, size_t pages);
void *get_phys_addr(void *vaddr);

#endif
