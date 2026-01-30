#ifndef _KERNEL_MEMCONST_H
#define _KERNEL_MEMCONST_H

#define PAGE_SIZE 4096
#define MAX_MEMORY (1024ULL * 1024ULL * 1024ULL * 4ULL) // 4GiB
#define GET_ADDR_PAGE(x) ((void *)(((uint32_t)(x) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1)))

// The number of page directory entries per page
#define PAGE_DIR_ENTRY_COUNT 1024

// The number of page table entries per page
#define PAGE_TABLE_ENTRY_COUNT 1024

// The size of memory a single page directory entry covers
#define PDE_SIZE (PAGE_TABLE_ENTRY_COUNT * PAGE_SIZE)

// Memory region for mapping I/O devices from 0xD0000000 to 0xDFFFFFFF
#define IO_MEM_START ((void *)0xD0000000UL)
#define IO_MEM_END ((void *)0xE0000000UL)

// The size of the I/O memory region
#define IO_MEM_SIZE (IO_MEM_END - IO_MEM_START)

// Memory region for kernel space from 0xC0000000 to 0xCFFFFFFF
#define KERNEL_MEM_START ((void *)0xC0000000UL)
#define KERNEL_MEM_END ((void *)0xD0000000UL)

// The size of the kernel memory region
#define KERNEL_MEM_SIZE (KERNEL_MEM_END - KERNEL_MEM_START)

// Memory region for user space from 0x00100000 to 0xBFFFFFFF
#define USER_MEM_START ((void *)0x00100000UL)
#define USER_MEM_END ((void *)0xC0000000UL)

// The size of the user memory region
#define USER_MEM_SIZE (USER_MEM_END - USER_MEM_START)

#define KERNEL_PHYS_ADDR(x) ((paddr_t)(((uintptr_t)(x)) - (uintptr_t)KERNEL_MEM_START))

#define GET_ADDR_PDE(x) (((uint32_t)(uintptr_t)(x) >> 22) & 0x3FF)
#define GET_ADDR_PTE(x) (((uint32_t)(uintptr_t)(x) >> 12) & 0x3FF)
#define STORE_ADDR(x) (((uint32_t)(uintptr_t)(x) >> 12) & 0xFFFFF)
#define GET_PDE_BASE_ADDR(pde) ((paddr_t)((pde).address << 12))
#define GET_PTE_BASE_ADDR(pte) ((paddr_t)((pte).address << 12))
#define NEXT_PAGE_ADDR(x) ((void *)((uint32_t)(uintptr_t)(x) + PAGE_SIZE))

#endif // _KERNEL_MEMCONST_H