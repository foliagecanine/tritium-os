#ifndef _KERNEL_PMM_H
#define _KERNEL_PMM_H

#include <kernel/multiboot.h>
#include <stddef.h>
#include <stdint.h>

extern void *_kernel_start;
extern void *_kernel_end;

#define kernel_start &_kernel_start
#define kernel_end   &_kernel_end

#define KERNEL_SIZE ((size_t)kernel_end - (size_t)kernel_start)
#if KERNEL_SIZE >= (4 * 1024 * 1024)
#error "Kernel size exceeds 4MB limit! You need to update paging.c to map more pages for the kernel, then update this limit."
#endif

typedef void* paddr_t;

#define PMM_BAD_ADDR ((paddr_t)(uintptr_t)(-1))

/**
 * Initialize the Physical Memory Manager with the given multiboot info.
 * 
 * @param mbi Pointer to the multiboot info structure.
 * @param mbi_base_addr Base address of the multiboot info structure.
 */
void pmm_init(volatile multiboot_info_t *mbi, void *mbi_base_addr);

/**
 * Allocate a single physical memory page.
 * 
 * @return Physical address of the allocated page, or PMM_BAD_ADDR on failure.
 */
paddr_t pmm_alloc_page();

/**
 * Free a single physical memory page.
 * 
 * @param page Physical address of the page to free.
 */
void pmm_free_page(paddr_t page);

/**
 * Allocate consecutive physical memory pages.
 * 
 * @param pages Number of pages to allocate.
 * @return Physical address of the allocated memory, or PMM_BAD_ADDR on failure.
 */
paddr_t pmm_alloc_seq_pages(size_t pages);

/**
 * Allocate consecutive physical memory pages at a specific physical address.
 * 
 * @param paddr Desired physical address to allocate.
 * @param pages Number of pages to allocate.
 * @return Physical address of the allocated memory, or PMM_BAD_ADDR on failure.
 */
paddr_t pmm_alloc_paddr(paddr_t paddr, size_t pages);

/**
 * Free consecutive physical memory pages at a specific physical address.
 * 
 * @param paddr Physical address of the memory to free.
 * @param pages Number of pages to free.
 */
void pmm_free_paddr(paddr_t paddr, size_t pages);

/**
 * Get the count of free physical memory pages.
 * 
 * @return Number of free physical memory pages.
 */
int pmm_get_free_page_count();

/**
 * Get the count of unallocated physical memory pages.
 */
size_t free_pages();

#endif // _KERNEL_PMM_H