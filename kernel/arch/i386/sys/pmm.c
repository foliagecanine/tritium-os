#include <kernel/pmm.h>
#include <kernel/mem.h>
#include <kernel/memconst.h>
#include <kernel/multiboot.h>
#include <kernel/kprint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>

#define UNITSIZE (sizeof(unsigned long) * 8)
#define UNIT_PAGE_SPAN (PAGE_SIZE * UNITSIZE)

static const char *mem_types[] = {"ERROR", "Available", "Reserved", "ACPI Reclaimable", "NVS", "Bad RAM"};

static size_t pmm_free_pages = 0;
static size_t pmm_total_pages = 0;

static unsigned long pmem_used[(MAX_MEMORY + UNIT_PAGE_SPAN - 1) / UNIT_PAGE_SPAN] __attribute__((aligned(PAGE_SIZE)));

static bool pmm_check_page(paddr_t addr) {
    uint32_t page = (uint32_t)(uintptr_t)addr;
    page /= PAGE_SIZE;

    return (bool)(pmem_used[page / UNITSIZE] & (1UL << (page % UNITSIZE)));
}

static bool pmm_check_cluster(paddr_t addr)
{
    uint32_t page = (uint32_t)(uintptr_t)addr;
    page /= PAGE_SIZE;
    return pmem_used[page / UNITSIZE] != (unsigned long)(~0UL);
}

static void dump_pmem_used() {
    kprint("[PMM] Dumping physical memory usage bitmap:");
    for (size_t i = 0; i < (MAX_MEMORY + UNIT_PAGE_SPAN - 1) / UNIT_PAGE_SPAN; i += 8) {
        if (!pmm_check_cluster((paddr_t)(uintptr_t)(i * UNIT_PAGE_SPAN))) {
            continue;
        }
        
        dprintf("  Unit %u-%u: 0x%08lX 0x%08lX 0x%08lX 0x%08lX 0x%08lX 0x%08lX 0x%08lX 0x%08lX\n", (unsigned int)i, (unsigned int)(i + 7), pmem_used[i], pmem_used[i + 1], pmem_used[i + 2], pmem_used[i + 3], pmem_used[i + 4], pmem_used[i + 5], pmem_used[i + 6], pmem_used[i + 7]);
    }
}

static void pmm_mark_page(paddr_t addr, bool used) {
    uint32_t page = (uint32_t)(uintptr_t)addr;
    page /= PAGE_SIZE;

    if (used) {
        pmem_used[page / UNITSIZE] |= (1UL << (page % UNITSIZE));
    } else {
        pmem_used[page / UNITSIZE] &= ~(1UL << (page % UNITSIZE));
    }
}

void pmm_init(volatile multiboot_info_t *mbi, void *mbi_base_addr) {
    kprint("[PMM] Initializing Physical Memory Manager...");

    pmm_free_pages = 0;
    pmm_total_pages = 0;

    // Mark all memory as used by default
    for (size_t i = 0; i < (MAX_MEMORY + UNIT_PAGE_SPAN - 1) / UNIT_PAGE_SPAN; i++) {
        pmem_used[i] = ~0UL;
    }

    multiboot_memory_map_t *mmap = (multiboot_memory_map_t *)(mbi_base_addr + (mbi->mmap_addr & 0xFFF));

    // Scan the memory map for areas that we can use for general purposes
    kprintf("[PMM] %u memory map entries found", mbi->mmap_length / sizeof(multiboot_memory_map_t));
    for (uint8_t i = 0; i < mbi->mmap_length / sizeof(multiboot_memory_map_t); i++) {
        uint32_t type = mmap[i].type;
        if (type > 5) {
            type = 2;
        }

        if (i > 0 && mmap[i].addr == 0) {
            break;
        }
        
        if (type == 1) {
            // Mark available memory as free
            for (uint32_t j = 0; j < mmap[i].len / PAGE_SIZE; j++)
            {
                pmm_mark_page((void *)(uintptr_t)(mmap[i].addr + j * PAGE_SIZE), false);
                pmm_free_pages++;
            }
        }

        pmm_total_pages += mmap[i].len / PAGE_SIZE;

        kprintf("%hhu: 0x%08llX+0x%08llX %s", i, mmap[i].addr, mmap[i].len, mem_types[type]);
    }

    // Mark kernel pages as used
    for (void *addr = KERNEL_PHYS_ADDR(kernel_start); addr < KERNEL_PHYS_ADDR(kernel_end); addr = (void *)((uint32_t)(uintptr_t)addr + PAGE_SIZE)) {
        pmm_mark_page(addr, true);
        pmm_free_pages--;
    }

    // Mark first 1MB as used
    for (void *addr = 0; addr < (void *)0x100000; addr = (void *)((uint32_t)(uintptr_t)addr + PAGE_SIZE)) {
        pmm_mark_page(addr, true);
        pmm_free_pages--;
    }

    dprintf("[PMM] Total Memory: %u KB (%u pages)\n", (unsigned int)(pmm_total_pages * PAGE_SIZE / 1024), (unsigned int)pmm_total_pages);
    dprintf("[PMM] Free Memory: %u KB (%u pages)\n", (unsigned int)(pmm_free_pages * PAGE_SIZE / 1024), (unsigned int)pmm_free_pages);
}

bool warn_claimed = true;

paddr_t pmm_alloc_page() {
    for (paddr_t check_addr = (paddr_t)PAGE_SIZE; check_addr != 0; check_addr += PAGE_SIZE) {
        if ((uintptr_t)check_addr % (UNITSIZE * PAGE_SIZE) == 0 && !pmm_check_cluster(check_addr)) {
            // All pages in this cluster are used, skip it

            // If we overflow, all memory is used.
            if (check_addr + UNITSIZE * PAGE_SIZE < check_addr) {
                break;
            }

            check_addr += UNITSIZE * PAGE_SIZE;
            continue;
        }
        
        // Check if we have a free page
        if (!pmm_check_page(check_addr)) {
            pmm_mark_page(check_addr, true);
            pmm_free_pages--;
            return check_addr;
        }
    }

    return PMM_BAD_ADDR;
}

void pmm_free_page(paddr_t page) {
    pmm_mark_page(page, false);
    pmm_free_pages++;
}

paddr_t pmm_alloc_seq_pages(size_t pages) {
    for (paddr_t check_addr = (paddr_t)PAGE_SIZE; check_addr != 0; check_addr += PAGE_SIZE) {
        if ((uintptr_t)check_addr % (UNITSIZE * PAGE_SIZE) == 0 && !pmm_check_cluster(check_addr)) {
            // All pages in this cluster are used, skip it

            // If we overflow, all memory is used.
            if (check_addr + UNITSIZE * PAGE_SIZE < check_addr) {
                break;
            }

            check_addr += UNITSIZE * PAGE_SIZE;
            continue;
        }
        
        // Check if we have enough consecutive free pages
        bool found = true;
        for (size_t i = 0; i < pages; i++) {
            if (pmm_check_page((void *)((uint32_t)(uintptr_t)check_addr + i * PAGE_SIZE))) {
                found = false;
                check_addr += i * PAGE_SIZE;
                break;
            }
        }
        
        // Allocate the pages if we found enough consecutive free pages
        if (found) {
            for (size_t i = 0; i < pages; i++) {
                pmm_mark_page((void *)((uint32_t)(uintptr_t)check_addr + i * PAGE_SIZE), true);
                pmm_free_pages--;
            }
            return check_addr;
        }
    }

    return PMM_BAD_ADDR;
}

paddr_t pmm_alloc_paddr(paddr_t paddr, size_t pages) {
    // Mark the pages as used
    for (size_t i = 0; i < pages; i++) {
        if (!pmm_check_page((void *)((uint32_t)(uintptr_t)paddr + i * PAGE_SIZE))) {
            pmm_mark_page((void *)((uint32_t)(uintptr_t)paddr + i * PAGE_SIZE), true);
            pmm_free_pages--;
        }
    }

    return paddr;
}

void pmm_free_paddr(paddr_t paddr, size_t pages) {
    for (size_t i = 0; i < pages; i++) {
        pmm_mark_page((void *)((uint32_t)(uintptr_t)paddr + i * PAGE_SIZE), false);
        pmm_free_pages++;
    }
}

size_t free_pages() {
    return pmm_free_pages;
}