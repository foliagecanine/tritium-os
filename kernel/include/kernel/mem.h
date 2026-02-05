#ifndef _KERNEL_MEM_H
#define _KERNEL_MEM_H

#include <stdint.h>
#include <kernel/multiboot.h>
#include <stdbool.h>
#include <stddef.h>
#include <kernel/pmm.h>
#include <kernel/memconst.h>

typedef struct pde_t {
	uint8_t present:1;
	uint8_t readwrite:1;
	uint8_t user:1;
	uint8_t writethru:1;
	uint8_t cachedisable:1;
	uint8_t access:1;
	uint8_t zero:1;
	uint8_t size:1;
	uint8_t ignore:4;
	uint32_t address:20;
} __attribute__((packed)) pde_t;

typedef struct pte_t {
	uint8_t present:1;
	uint8_t readwrite:1;
	uint8_t user:1;
	uint8_t writethru:1;
	uint8_t cached:1;
	uint8_t access:1;
	uint8_t dirty:1;
	uint8_t zero:1;
	uint8_t ignore:4;
	uint32_t address:20;
} __attribute__((packed)) pte_t;

typedef struct page_directory_t {
	pde_t phys_dir[1024];
	pte_t *virt_dir[1024];
} __attribute__((aligned(PAGE_SIZE))) __attribute__((packed)) page_directory_t;

#define PAGE_DIRECTORY_PAGES ((sizeof(page_directory_t) + PAGE_SIZE - 1) / PAGE_SIZE)

#define PAGE_PERM_USER 0x1
#define PAGE_PERM_WRITE 0x2
#define PAGE_STAT_NOCACHE 0x4
#define PAGE_STAT_WRITETHRU 0x8
#define PAGE_ERROR_NOT_PRESENT (uint8_t)(-1)

typedef void* kvaddr_t;
typedef void* iovaddr_t;
typedef void* uvaddr_t;

/**
 * Initialize paging subsystem.
 * 
 * @param mbi Pointer to multiboot info structure.
 */
void init_paging(multiboot_info_t *mbi);

/**
 * Map a kernel page.
 * 
 * @param vaddr Virtual address to map.
 * @param paddr Physical address to map to.
 */
void kmap_page(void *vaddr, paddr_t paddr);

/**
 * Unmap a kernel page.
 * 
 * @param vaddr Virtual address to unmap.
 */
void kunmap_page(void *vaddr);

/**
 * Map an I/O page.
 * 
 * @param vaddr Virtual address to map.
 * @param paddr Physical address to map to.
 */
void iomap_page(void *vaddr, paddr_t paddr);

/**
 * Unmap an I/O page.
 * 
 * @param vaddr Virtual address to unmap.
 */
void iounmap_page(void *vaddr);

/**
 * Allocate and map kernel pages.
 * 
 * @param count Number of pages to allocate.
 */
kvaddr_t kalloc_pages(size_t count);

/**
 * Allocate and map kernel pages with sequential physical memory.
 * 
 * @param count Number of pages to allocate.
 */
kvaddr_t kalloc_sequential(size_t count);

/**
 * Allocate and map I/O pages.
 * 
 * @param paddr Physical address to map.
 * @param count Number of pages to allocate.
 * 
 * @returns Virtual address of the mapped I/O memory (note: if paddr is not page aligned, the offset within the page is preserved).
 */
iovaddr_t ioalloc_pages(paddr_t paddr, size_t count);

/**
 * Free kernel pages.
 * 
 * @param vaddr Virtual address of the first page to free.
 * @param count Number of pages to free.
 */
void kfree_pages(kvaddr_t vaddr, size_t count);

/**
 * Free I/O pages.
 * 
 * @param vaddr Virtual address of the first page to free.
 * @param count Number of pages to free.
 */
void iofree_pages(iovaddr_t vaddr, size_t count);

/**
 * Get physical address mapped to a virtual kernel address.
 * 
 * @param vaddr Virtual address to get the physical address for.
 * @return Physical address mapped to the given virtual address.
 */
paddr_t get_kphys(kvaddr_t vaddr);

/**
 * Get physical address mapped to a virtual I/O address.
 * 
 * @param vaddr Virtual address to get the physical address for.
 * @return Physical address mapped to the given virtual address.
 */
paddr_t get_iophys(iovaddr_t vaddr);

/**
 * Get the current page tables.
 * 
 * @return The current page directory.
 */
page_directory_t *get_current_tables();

/**
 * Switch to the given page tables.
 * 
 * @param new_directory The new page directory to activate.
 */
void switch_tables(page_directory_t *new_directory);

/**
 * Switch to the kernel page tables.
 */
void use_kernel_tables();

/**
 * Clone the current page tables.
 * 
 * @return A new page directory that is a clone of the current one.
 */
page_directory_t *clone_tables();

/**
 * Check if a kernel virtual address is currently mapped.
 * 
 * @param vaddr The kernel virtual address to check.
 * @return true if the address is mapped, false otherwise.
 */
bool is_kernel_vaddr_mapped(kvaddr_t vaddr);

/**
 * Back user pages in the target directory with new physical memory.
 * 
 * @param target_directory The page directory to clone from.
 */
void clone_user_pages(page_directory_t *target_directory);

/**
 * Map an address to user space.
 * 
 * @param vaddr Virtual address in user space
 * @return True if successful
 */
bool map_user_page(uvaddr_t vaddr);

/**
 * Map an address in user space to a given physical address.
 * 
 * @param vaddr Virtual address in user space.
 * @param paddr Physical address to map to.
 * @return True if successful
 */
bool map_user_page_paddr(uvaddr_t vaddr, paddr_t paddr);

/**
 * Unmap an address from user space.
 * 
 * @param vaddr The virtual address to unmap
 * @return True if successful
 */
bool unmap_user_page(uvaddr_t vaddr);

/**
 * Free the current page directory and switch to kernel tables
 * 
 * @param pagedir The page directory to free
 */
void free_current_page_directory();

/**
 * Mark a page as writable or read-only.
 */
void mark_write(uvaddr_t vaddr, bool writable);

/**
 * Get the permissions of a page.
 * 
 * @param vaddr Virtual address to check.
 * @return Permissions bitmask.
 */
uint8_t get_page_permissions(void *vaddr);

/**
 * Identity map a range of pages.
 * 
 * @param vaddr Starting virtual address.
 * @param count Number of pages to map.
 */
void identity_map_pages(paddr_t vaddr, size_t count);

/**
 * Free a range of identity-mapped pages.
 * 
 * @param vaddr Starting virtual address.
 * @param count Number of pages to free.
 */
void identity_free_pages(paddr_t vaddr, size_t count);

#endif /* _KERNEL_MEM_H */
