#include <kernel/ksetup.h>
#include <kernel/mem.h>
#include <kernel/memconst.h>
#include <kernel/syscalls.h>
#include <kernel/sysfunc.h>
#include <kernel/pmm.h>
#include <kernel/kprint.h>
#include <stdbool.h>

// 256 KiB each for the tables, each mapping a total of 256MiB of virtual memory
static pte_t kernel_page_tables[KERNEL_MEM_SIZE / PAGE_SIZE] __attribute__((aligned(PAGE_SIZE)));
static pte_t io_page_tables[IO_MEM_SIZE / PAGE_SIZE] __attribute__((aligned(PAGE_SIZE)));

static page_directory_t *page_directory;
static page_directory_t kernel_page_directory;

static char bootstrap_window_page[PAGE_SIZE] __attribute__((aligned(PAGE_SIZE)));
static char vga_buffer[PAGE_SIZE] __attribute__((aligned(PAGE_SIZE)));

/**
 * Load the given CR3 value into the CPU.
 */
static inline void load_cr3(paddr_t cr3) {
    asm volatile("mov %0, %%cr3" : : "r"((uint32_t)(uintptr_t)cr3) : "memory");
}

/**
 * Invalidate a single page in the TLB.
 */
static inline void invalidate_page(void *vaddr) {
    asm volatile("invlpg (%0)" : : "r"(vaddr) : "memory");
}

/**
 * Flush the entire TLB by reloading CR3.
 */
// static void flush_tlb() {
//     uint32_t cr3;
//     asm volatile("mov %%cr3, %0" : "=r"(cr3));
//     asm volatile("mov %0, %%cr3" : : "r"(cr3) : "memory");
// }

/**
 * Set a page directory entry in the given page directory.
 * 
 * @param page_directory Pointer to the page directory.
 * @param entries The virtual address of the page directory entries.
 * @param entry The page directory entry to set.
 */
static void set_page_directory_entry(page_directory_t *page_directory, void *vaddr, pte_t *entries, pde_t entry) {
    uint32_t index = GET_ADDR_PDE(vaddr);
    page_directory->phys_dir[index] = entry;
    page_directory->virt_dir[index] = entries;
}

/**
 * Get a kernel page table entry from the kernel page tables.
 * 
 * @param vaddr Virtual address to get the entry for.
 * @return The page table entry.
 */
static pte_t *get_kernel_pte(kvaddr_t vaddr) {
    if (vaddr < KERNEL_MEM_START || vaddr >= KERNEL_MEM_END) {
        dprintf("Failed to get kernel PTE for vaddr %p\n", vaddr);
        dprintf("  vaddr=%p out of range of kernel space\n", vaddr);
        PANIC("Attempted to get kernel page table entry for address outside kernel space");
    }

    void *relative_vaddr = (void *)((uintptr_t)vaddr - (uintptr_t)KERNEL_MEM_START);
    size_t full_map_index = (GET_ADDR_PDE(relative_vaddr) * PAGE_TABLE_ENTRY_COUNT) + GET_ADDR_PTE(relative_vaddr);
    return &kernel_page_tables[full_map_index];
}

/**
 * Set a kernel page table entry in the kernel page tables.
 */
static void set_kernel_pte(kvaddr_t vaddr, pte_t entry) {
    if (vaddr < KERNEL_MEM_START || vaddr >= KERNEL_MEM_END) {
        dprintf("Failed to set kernel PTE for vaddr %p\n", vaddr);
        dprintf("  vaddr=%p out of range of kernel space\n", vaddr);
        PANIC("Attempted to set kernel page table entry for address outside kernel space");
    }

    void *relative_vaddr = (void *)((uintptr_t)vaddr - (uintptr_t)KERNEL_MEM_START);
    size_t full_map_index = (GET_ADDR_PDE(relative_vaddr) * PAGE_TABLE_ENTRY_COUNT) + GET_ADDR_PTE(relative_vaddr);

    // Initialize the page directory entry if not present
    if (page_directory->phys_dir[GET_ADDR_PDE(vaddr)].present == 0) {
        pde_t new_pde = {
            .present = 1,
            .readwrite = 1,
            .user = 0,
            .writethru = 0,
            .cachedisable = 0,
            .access = 0,
            .zero = 0,
            .size = 0,
            .ignore = 0,
            .address = STORE_ADDR(KERNEL_PHYS_ADDR(&kernel_page_tables[full_map_index]))
        };

        set_page_directory_entry(page_directory, vaddr, (pte_t *)&kernel_page_tables[full_map_index - GET_ADDR_PTE(vaddr)], new_pde);
    }

    kernel_page_tables[full_map_index] = entry;
}

void kmap_page(kvaddr_t vaddr, paddr_t paddr) {
    if (vaddr < KERNEL_MEM_START || vaddr >= KERNEL_MEM_END) {
        dprintf("Failed to map kernel page at vaddr=%p\n", vaddr);
        dprintf("  vaddr=%p out of range of kernel space\n", vaddr);
        PANIC("Attempted to map kernel page outside kernel space");
    }

    pte_t entry = {
        .present = 1,
        .readwrite = 1,
        .user = 0,
        .writethru = 0,
        .cached = 1,
        .access = 0,
        .dirty = 0,
        .zero = 0,
        .ignore = 0,
        .address = STORE_ADDR((void *)(uintptr_t)paddr)
    };
    
    set_kernel_pte(vaddr, entry);
    invalidate_page(vaddr);
}

void kunmap_page(void *vaddr) {
    if (vaddr < KERNEL_MEM_START || vaddr >= KERNEL_MEM_END) {
        dprintf("Failed to unmap kernel page at vaddr=%p\n", vaddr);
        dprintf("  vaddr=%p out of range of kernel space\n", vaddr);
        PANIC("Attempted to unmap kernel page outside kernel space");
    }

    void *relative_vaddr = (void *)((uintptr_t)vaddr - (uintptr_t)KERNEL_MEM_START);

    size_t full_map_index = (GET_ADDR_PDE(relative_vaddr) * PAGE_TABLE_ENTRY_COUNT) + GET_ADDR_PTE(relative_vaddr);
    kernel_page_tables[full_map_index].present = 0;
    invalidate_page(vaddr);
}

/**
 * Get an I/O page table entry from the I/O page tables.
 * 
 * @param vaddr Virtual address to get the entry for.
 * @return The page table entry.
 */
static pte_t *get_io_pte(iovaddr_t vaddr) {
    if (vaddr < IO_MEM_START || vaddr >= IO_MEM_END) {
        dprintf("Failed to get I/O PTE for vaddr %p\n", vaddr);
        dprintf("  vaddr=%p out of range of I/O space\n", vaddr);
        PANIC("Attempted to get I/O page table entry for address outside I/O space");
    }

    void *relative_vaddr = (void *)((uintptr_t)vaddr - (uintptr_t)IO_MEM_START);
    size_t full_map_index = (GET_ADDR_PDE(relative_vaddr) * PAGE_TABLE_ENTRY_COUNT) + GET_ADDR_PTE(relative_vaddr);
    return &kernel_page_tables[full_map_index];
}

/**
 * Set an I/O page table entry in the I/O page tables.
 */
static void set_io_pte(iovaddr_t vaddr, pte_t entry) {
    if (vaddr < IO_MEM_START || vaddr >= IO_MEM_END) {
        dprintf("Failed to set I/O PTE for vaddr %p\n", vaddr);
        dprintf("  vaddr=%p out of range of I/O space\n", vaddr);
        PANIC("Attempted to set I/O page table entry for address outside I/O space");
    }

    void *relative_vaddr = (void *)((uintptr_t)vaddr - (uintptr_t)IO_MEM_START);
    size_t full_map_index = (GET_ADDR_PDE(relative_vaddr) * PAGE_TABLE_ENTRY_COUNT) + GET_ADDR_PTE(relative_vaddr);

    // Initialize the page directory entry if not present
    if (page_directory->phys_dir[GET_ADDR_PDE(vaddr)].present == 0) {
        pde_t new_pde = {
            .present = 1,
            .readwrite = 1,
            .user = 0,
            .writethru = 0,
            .cachedisable = 0,
            .access = 0,
            .zero = 0,
            .size = 0,
            .ignore = 0,
            .address = STORE_ADDR(KERNEL_PHYS_ADDR(&io_page_tables[full_map_index]))
        };

        set_page_directory_entry(page_directory, vaddr, (pte_t *)&io_page_tables[full_map_index - GET_ADDR_PTE(vaddr)], new_pde);
    }

    io_page_tables[full_map_index] = entry;
}

void iomap_page(iovaddr_t vaddr, paddr_t paddr) {
    if (vaddr < IO_MEM_START || vaddr >= IO_MEM_END) {
        dprintf("Failed to map I/O page at vaddr=%p\n", vaddr);
        dprintf("  vaddr=%p out of range of I/O space\n", vaddr);
        PANIC("Attempted to map I/O page outside I/O space");
    }

    pte_t entry = {
        .present = 1,
        .readwrite = 1,
        .user = 0,
        .writethru = 1, // I/O pages are write-through
        .cached = 0, // I/O pages are not cached
        .access = 0,
        .dirty = 0,
        .zero = 0,
        .ignore = 0,
        .address = STORE_ADDR((void *)(uintptr_t)paddr)
    };
    
    set_io_pte(vaddr, entry);
    invalidate_page(vaddr);
}

void iounmap_page(iovaddr_t vaddr) {
    if (vaddr < IO_MEM_START || vaddr >= IO_MEM_END) {
        dprintf("Failed to unmap I/O page at vaddr=%p\n", vaddr);
        dprintf("  vaddr=%p out of range of I/O space\n", vaddr);
        PANIC("Attempted to unmap I/O page outside I/O space");
    }

    void *relative_vaddr = (void *)((uintptr_t)vaddr - (uintptr_t)IO_MEM_START);

    size_t full_map_index = (GET_ADDR_PDE(relative_vaddr) * PAGE_TABLE_ENTRY_COUNT) + GET_ADDR_PTE(relative_vaddr);
    io_page_tables[full_map_index].present = 0;
    
    invalidate_page(vaddr);
}

void init_paging(multiboot_info_t *mbi) {
    // Currently kernel is identity mapped in the bootloader stage and paging is enabled.
    // We need to bootstrap the PMM, then we can set up proper paging structures.
    kprint("[PAGING] Initializing paging");

    // Allocate page directory and page tables right after the kernel in memory.
    memset(&kernel_page_directory, 0, sizeof(kernel_page_directory));
    page_directory = &kernel_page_directory;

    dprintf("[PAGING] Setting up kernel page directory at 0x%X\n", page_directory);

    // Map kernel physical memory to virtual memory in the bootstrap kernel page table
    dprintf("[PAGING] Mapping kernel memory starting at 0x%X\n", KERNEL_PHYS_ADDR(kernel_start));
    for (kvaddr_t addr = kernel_start; addr < (kvaddr_t)kernel_end; addr = NEXT_PAGE_ADDR(addr)) {
        pte_t entry = {
            .present = 1,
            .readwrite = 1,
            .user = 0,
            .writethru = 0,
            .cached = 1,
            .access = 0,
            .dirty = 0,
            .zero = 0,
            .ignore = 0,
            .address = STORE_ADDR(KERNEL_PHYS_ADDR(addr))
        };
        set_kernel_pte(addr, entry);
    }

    // Load the new page directory
    load_cr3(KERNEL_PHYS_ADDR(kernel_page_directory.phys_dir));

    // Set VGA buffer to mapped page so we can print debug info during boot
    pte_t vga_entry = {
        .present = 1,
        .readwrite = 1,
        .user = 0,
        .writethru = 1,
        .cached = 0,
        .access = 0,
        .dirty = 0,
        .zero = 0,
        .ignore = 0,
        .address = STORE_ADDR(0xB8000)
    };

    set_kernel_pte((kvaddr_t)vga_buffer, vga_entry);
    set_vga_buffer_address((void *)((uintptr_t)vga_buffer));
    invalidate_page(vga_buffer);
    
    // Identity map mbi for pmm_init.
    // Use secret page to avoid needing to use the pmm before it's initialized.
    // We use bootstrap_window_page to ensure we have virtual memory to map to.
    kmap_page(bootstrap_window_page, GET_ADDR_PAGE(mbi));
    
    // Initialize PMM
    pmm_init((volatile multiboot_info_t *)(bootstrap_window_page + ((uintptr_t)mbi & 0xFFF)), bootstrap_window_page);
    
    // Unmap the secret page
    kunmap_page(bootstrap_window_page);
}

kvaddr_t find_free_kernel_pages(size_t pages) {
    for (kvaddr_t addr = KERNEL_MEM_START; addr < KERNEL_MEM_END; addr += PAGE_SIZE) {
        bool block_free = true;
        for (size_t j = 0; j < pages; j++) {
            pte_t *pte = get_kernel_pte(addr + (j * PAGE_SIZE));
            if (pte->present) {
                block_free = false;
                break;
            }
        }
        if (block_free) {
            return addr;
        }
    }

    return NULL;
}

iovaddr_t find_free_io_pages(size_t pages) {
    for (iovaddr_t addr = IO_MEM_START; addr < IO_MEM_END; addr += PAGE_SIZE) {
        bool block_free = true;
        for (size_t j = 0; j < pages; j++) {
            pte_t *pte = get_io_pte(addr + (j * PAGE_SIZE));
            if (pte->present) {
                block_free = false;
                break;
            }
        }
        if (block_free) {
            return addr;
        }
    }

    return NULL;
}

kvaddr_t kalloc_pages(size_t count) {
    // Find free virtual pages in kernel space
    kvaddr_t vaddr = find_free_kernel_pages(count);

    if (vaddr == NULL) {
        dprintf("Failed to allocate %zu kernel pages\n", count);
        dprintf("  out of virtual memory\n");
        PANIC("Out of kernel virtual memory while allocating kernel page");
    }

    for (size_t i = 0; i < count; i++) {
        paddr_t paddr = pmm_alloc_page();

        if (paddr == NULL) {
            dprintf("Failed to allocate kernel page %zu/%zu\n", i, count);
            dprintf("  out of physical memory\n");
            PANIC("Failed to allocate kernel page");
        }

        kmap_page(vaddr + (i * PAGE_SIZE), paddr);
    }

    return vaddr;
}

kvaddr_t kalloc_sequential(size_t count) {
    // Find free virtual pages in kernel space
    kvaddr_t vaddr = find_free_kernel_pages(count);

    if (vaddr == NULL) {
        dprintf("Failed to allocate %zu sequential kernel pages\n", count);
        dprintf("  out of virtual memory\n");
        PANIC("Out of kernel virtual memory while allocating kernel page");
    }

    paddr_t paddr = pmm_alloc_seq_pages(count);

    if (paddr == NULL) {
        dprintf("Failed to allocate %zu sequential kernel pages\n", count);
        dprintf("  out of physical memory\n");
        PANIC("Failed to allocate sequential kernel pages");
    }

    for (size_t i = 0; i < count; i++) {
        kmap_page(vaddr + (i * PAGE_SIZE), paddr + (i * PAGE_SIZE));
    }
    
    return vaddr;
}

iovaddr_t ioalloc_pages(paddr_t paddr, size_t count) {
    // Find a free virtual page in I/O space
    iovaddr_t vaddr = find_free_io_pages(count);

    if (vaddr == NULL) {
        dprintf("Failed to allocate %zu I/O pages\n", count);
        dprintf("  out of virtual memory\n");
        PANIC("Out of I/O virtual memory while allocating I/O page");
    }

    iomap_page(vaddr, paddr);
    return vaddr + ((uintptr_t)paddr & 0xFFF);
}

void kfree_pages(kvaddr_t vaddr, size_t count) {
    if (vaddr < KERNEL_MEM_START || vaddr >= KERNEL_MEM_END) {
        dprintf("Failed to free kernel pages at vaddr=%p\n", vaddr);
        dprintf("  vaddr=%p out of range of kernel space\n", vaddr);
        PANIC("Attempted to free kernel pages outside kernel space");
    }

    for (size_t i = 0; i < count; i++) {
        pte_t *pte = get_kernel_pte(vaddr + (i * PAGE_SIZE));

        if (!pte->present) {
            dprintf("Failed to free kernel page at vaddr=%p\n", vaddr + (i * PAGE_SIZE));
            dprintf("  page not present\n");
            PANIC("Attempted to free unallocated kernel page");
        }

        pmm_free_page(GET_PTE_BASE_ADDR(*pte));
        kunmap_page(vaddr + (i * PAGE_SIZE));
    }
}

void iofree_pages(iovaddr_t vaddr, size_t count) {
    if (vaddr < IO_MEM_START || vaddr >= IO_MEM_END) {
        dprintf("Failed to free I/O pages at vaddr=%p\n", vaddr);
        dprintf("  vaddr=%p out of range of I/O space\n", vaddr);
        PANIC("Attempted to free I/O pages outside I/O space");
    }

    for (size_t i = 0; i < count; i++) {
        pte_t *pte = get_io_pte(vaddr + (i * PAGE_SIZE));

        if (!pte->present) {
            PANIC("Attempted to free unallocated I/O page");
        }

        iounmap_page(vaddr + (i * PAGE_SIZE));
    }
}

paddr_t get_kphys(kvaddr_t vaddr) {
    if (vaddr < KERNEL_MEM_START || vaddr >= KERNEL_MEM_END) {
        dprintf("Failed to get kernel physical address for vaddr %p\n", vaddr);
        dprintf("  vaddr=%p out of range of kernel space\n", vaddr);
        PANIC("Attempted to get physical address for kernel address outside kernel space");
    }

    pte_t *pte = get_kernel_pte(vaddr);

    if (!pte->present) {
        dprintf("Failed to get kernel physical address for vaddr %p\n", vaddr);
        dprintf("  virtual address is not mapped\n");
        PANIC("Virtual address is not mapped");
    }

    return GET_PTE_BASE_ADDR(*pte);
}

paddr_t get_iophys(iovaddr_t vaddr) {
    if (vaddr < IO_MEM_START || vaddr >= IO_MEM_END) {
        dprintf("Failed to get I/O physical address for vaddr %p\n", vaddr);
        dprintf("  vaddr=%p out of range of I/O space\n", vaddr);
        PANIC("Attempted to get physical address for I/O address outside I/O space");
    }

    pte_t *pte = get_io_pte(vaddr);

    if (!pte->present) {
        dprintf("Failed to get I/O physical address for vaddr %p\n", vaddr);
        dprintf("  virtual address is not mapped\n");
        PANIC("Virtual address is not mapped");
    }

    return GET_PTE_BASE_ADDR(*pte);
}

page_directory_t *get_current_tables() {
    return page_directory;
}

void switch_tables(page_directory_t *new_directory) {
    page_directory = new_directory;
    paddr_t dir_paddr = get_kphys(new_directory->phys_dir); // Ensure the new directory is mapped
    load_cr3(dir_paddr);
}

void use_kernel_tables() {
    switch_tables(&kernel_page_directory);
}

page_directory_t *clone_tables() {
    page_directory_t *new_directory = kalloc_pages(PAGE_DIRECTORY_PAGES);
    memcpy(new_directory, page_directory, sizeof(page_directory_t));

    // Copy user page tables
    for (size_t i = GET_ADDR_PDE(USER_MEM_START); i < GET_ADDR_PDE(USER_MEM_END); i++) {
        if (new_directory->phys_dir[i].present) {
            kvaddr_t new_table = kalloc_pages(1);
            memcpy(new_table, page_directory->virt_dir[i], PAGE_SIZE);

            new_directory->virt_dir[i] = (pte_t *)new_table;
            new_directory->phys_dir[i].address = STORE_ADDR(get_kphys(new_table));
        }
    }

    return new_directory;
}

bool map_user_page(uvaddr_t vaddr) {
    if (vaddr >= USER_MEM_END || vaddr < USER_MEM_START) {
        dprintf("Failed to map user page at vaddr=%p\n", vaddr);
        dprintf("  vaddr=%p out of range of user space\n", vaddr);
        return false;
    }

    // Check to see if we have a PDE for this page
    if (page_directory->phys_dir[GET_ADDR_PDE(vaddr)].present == 0) {
        // Allocate a kernel page for this PDE
        kvaddr_t new_table = kalloc_pages(1);
        memset(new_table, 0, PAGE_SIZE);

        // Create a new PDE
        pde_t new_pde = {
            .present = 1,
            .readwrite = 1,
            .user = 1,
            .writethru = 0,
            .cachedisable = 0,
            .access = 0,
            .zero = 0,
            .size = 0,
            .ignore = 0,
            .address = STORE_ADDR(get_kphys(new_table))
        };

        set_page_directory_entry(page_directory, vaddr, (pte_t *)new_table, new_pde);
    }

    // Get the virtual address of the page tables for the vaddr
    pte_t *pt_entries = page_directory->virt_dir[GET_ADDR_PDE(vaddr)];
    pte_t *target_pte = &pt_entries[GET_ADDR_PTE(vaddr)];

    paddr_t phys_backing = pmm_alloc_page();

    pte_t new_pte = {
        .present = 1,
        .readwrite = 1,
        .user = 1,
        .writethru = 0,
        .cached = 0,
        .access = 0,
        .dirty = 0,
        .zero = 0,
        .ignore = 0,
        .address = STORE_ADDR(phys_backing)
    };

    *target_pte = new_pte;

    invalidate_page(vaddr);

    return true;
}

bool map_user_page_paddr(uvaddr_t vaddr, paddr_t paddr) {
    if (vaddr >= USER_MEM_END || vaddr < USER_MEM_START) {
        dprintf("Failed to map user page at vaddr=%p\n", vaddr);
        dprintf("  vaddr=%p out of range of user space\n", vaddr);
        return false;
    }

    // Check to see if we have a PDE for this page
    if (page_directory->phys_dir[GET_ADDR_PDE(vaddr)].present == 0) {
        // Allocate a kernel page for this PDE
        kvaddr_t new_table = kalloc_pages(1);
        memset(new_table, 0, PAGE_SIZE);

        // Create a new PDE
        pde_t new_pde = {
            .present = 1,
            .readwrite = 1,
            .user = 1,
            .writethru = 0,
            .cachedisable = 0,
            .access = 0,
            .zero = 0,
            .size = 0,
            .ignore = 0,
            .address = STORE_ADDR(get_kphys(new_table))
        };

        set_page_directory_entry(page_directory, vaddr, (pte_t *)new_table, new_pde);
    }

    // Get the virtual address of the page tables for the vaddr
    pte_t *pt_entries = page_directory->virt_dir[GET_ADDR_PDE(vaddr)];
    pte_t *target_pte = &pt_entries[GET_ADDR_PTE(vaddr)];

    pte_t new_pte = {
        .present = 1,
        .readwrite = 1,
        .user = 1,
        .writethru = 0,
        .cached = 0,
        .access = 0,
        .dirty = 0,
        .zero = 0,
        .ignore = 0,
        .address = STORE_ADDR(paddr)
    };

    *target_pte = new_pte;

    invalidate_page(vaddr);

    return true;
}

bool unmap_user_page(uvaddr_t vaddr) {
    if (vaddr >= USER_MEM_END || vaddr < USER_MEM_START) {
        dprintf("Failed to unmap user page at vaddr=%p\n", vaddr);
        dprintf("  vaddr=%p out of range of user space\n", vaddr);
        return false;
    }

    if (page_directory->phys_dir[GET_ADDR_PDE(vaddr)].present == 0) {
        dprintf("Failed to unmap user page at vaddr=%p\n", vaddr);
        dprintf("  vaddr=%p page directory entry not present\n", vaddr);
        return false;
    }

    pte_t *pt_entries = page_directory->virt_dir[GET_ADDR_PDE(vaddr)];
    pte_t *target_entry = &pt_entries[GET_ADDR_PTE(vaddr)];
    
    if (target_entry->present == 0) {
        dprintf("Failed to unmap user page at vaddr=%p\n", vaddr);
        dprintf("  vaddr=%p page table entry not present\n", vaddr);
        return false;
    }

    pmm_free_page(GET_PTE_BASE_ADDR(*target_entry));
    *target_entry = (pte_t) {0};
    invalidate_page(vaddr);

    // Reclaim the page directory entry if all pages within it are free
    bool all_free = true;
    for (size_t i = 0; i < PAGE_TABLE_ENTRY_COUNT; i++) {
        if (pt_entries[i].present) {
            all_free = false;
            break;
        }
    }

    if (all_free) {
        page_directory->phys_dir[GET_ADDR_PDE(vaddr)] = (pde_t) {0};
        kfree_pages(page_directory->virt_dir[GET_ADDR_PDE(vaddr)], 1);
        page_directory->virt_dir[GET_ADDR_PDE(vaddr)] = NULL;
    }

    return true;
}

void free_current_page_directory() {
    // Free the user pages (kernel and I/O pages are preserved)
    for (uvaddr_t addr = USER_MEM_START; addr < USER_MEM_END; addr += PAGE_SIZE) {
        if (page_directory->phys_dir[GET_ADDR_PDE(addr)].present == 0) {
            continue;
        }

        pte_t *pt_entries = page_directory->virt_dir[GET_ADDR_PDE(addr)];
        pte_t *target_entry = &pt_entries[GET_ADDR_PTE(addr)];

        if (target_entry->present == 0 || target_entry->user == 0) {
            continue;
        }

        // Free the physical page
        pmm_free_page(GET_PTE_BASE_ADDR(*target_entry));
    }

    // Free the page directory tables
    for (uvaddr_t addr = USER_MEM_START; addr < USER_MEM_END; addr += PDE_SIZE) {
        if (page_directory->phys_dir[GET_ADDR_PDE(addr)].present == 0) {
            continue;
        }

        pte_t *pt_entries = page_directory->virt_dir[GET_ADDR_PDE(addr)];

        // Free the page table
        kfree_pages((kvaddr_t)pt_entries, 1);
    }

    page_directory_t *target_pd = page_directory;

    use_kernel_tables();

    // Free the actual page directory pages
    kfree_pages(target_pd, PAGE_DIRECTORY_PAGES);
}

void clone_user_pages(page_directory_t *target_directory) {
    for (uvaddr_t addr = USER_MEM_START; addr < USER_MEM_END; ) {
        if (target_directory->phys_dir[GET_ADDR_PDE(addr)].present == 0) {
            addr = (uvaddr_t)(((uintptr_t)addr & ~(PDE_SIZE - 1)) + PDE_SIZE);
            continue;
        }

        pte_t *pt_entries = target_directory->virt_dir[GET_ADDR_PDE(addr)];
        pte_t *target_entry = &pt_entries[GET_ADDR_PTE(addr)];

        if (target_entry->present == 0 || target_entry->user == 0) {
            addr = (uvaddr_t)((uintptr_t)addr + PAGE_SIZE);
            continue;
        }

        // Back the page with a new physical page
        paddr_t new_phys = pmm_alloc_page();

        // Copy the contents of the source page to the new page
        kvaddr_t source_kvaddr = find_free_kernel_pages(1);
        kmap_page(source_kvaddr, GET_PTE_BASE_ADDR(*target_entry));
        kvaddr_t target_kvaddr = find_free_kernel_pages(1);
        kmap_page(target_kvaddr, new_phys);

        memcpy((void *)target_kvaddr, (void *)source_kvaddr, PAGE_SIZE);
        kunmap_page(source_kvaddr);
        kunmap_page(target_kvaddr);

        // Set the page in the target directory to use the new paddr
        target_entry->address = STORE_ADDR(new_phys);

        // If we are currently using the target directory, invalidate the page
        if (page_directory == target_directory) {
            invalidate_page(addr);
        }

        addr = (uvaddr_t)((uintptr_t)addr + PAGE_SIZE);
    }
}

void mark_write(uvaddr_t vaddr, bool writable) {
    if (vaddr >= USER_MEM_END || vaddr < USER_MEM_START) {
        dprintf("Failed to mark write on user page at vaddr=%p\n", vaddr);
        dprintf("  vaddr=%p out of range of user space\n", vaddr);
        PANIC("Attempted to mark write on address outside user space");
    }

    if (page_directory->phys_dir[GET_ADDR_PDE(vaddr)].present == 0) {
        dprintf("Failed to mark write on user page at vaddr=%p\n", vaddr);
        dprintf("  vaddr=%p page directory entry not present\n", vaddr);
        PANIC("Attempted to mark write on unmapped page");
    }

    pte_t *pt_entries = page_directory->virt_dir[GET_ADDR_PDE(vaddr)];
    pte_t *target_entry = &pt_entries[GET_ADDR_PTE(vaddr)];

    if (target_entry->present == 0) {
        dprintf("Failed to mark write on user page at vaddr=%p\n", vaddr);
        dprintf("  vaddr=%p page table entry not present\n", vaddr);
        PANIC("Attempted to mark write on unmapped page");
    }

    target_entry->readwrite = writable ? 1 : 0;
    invalidate_page(vaddr);
}

bool is_kernel_vaddr_mapped(kvaddr_t vaddr) {
    if (vaddr < KERNEL_MEM_START || vaddr >= KERNEL_MEM_END) {
        return false;
    }

    uint32_t pd_index = GET_ADDR_PDE(vaddr);
    uint32_t pt_index = GET_ADDR_PTE(vaddr);

    if (kernel_page_directory.phys_dir[pd_index].present == 0) {
        return false;
    }

    pte_t *pt_entries = kernel_page_directory.virt_dir[pd_index];
    if (pt_entries == NULL) {
        return false;
    }

    return pt_entries[pt_index].present != 0;
}

uint8_t get_page_permissions(void *vaddr) {
    if (page_directory->phys_dir[GET_ADDR_PDE(vaddr)].present == 0) {
        return PAGE_ERROR_NOT_PRESENT;
    }
        
    pte_t *pt_entries = page_directory->virt_dir[GET_ADDR_PDE(vaddr)];
    
    if (!is_kernel_vaddr_mapped(pt_entries)) {
        return PAGE_ERROR_NOT_PRESENT;
    }
    
    pte_t *target_entry = &pt_entries[GET_ADDR_PTE(vaddr)];
    
    if (target_entry->present == 0) {
        return PAGE_ERROR_NOT_PRESENT;
    }
    
    uint8_t permissions = 0;

    if (target_entry->user) {
        permissions |= PAGE_PERM_USER;
    }

    if (target_entry->readwrite) {
        permissions |= PAGE_PERM_WRITE;
    }

    if (!target_entry->cached) {
        permissions |= PAGE_STAT_NOCACHE;
    }

    if (target_entry->writethru) {
        permissions |= PAGE_STAT_WRITETHRU;
    }

    return permissions;
}

bool identity_map(paddr_t vaddr) {
    paddr_t paddr = vaddr;

    // Ensure we're not mapping kernel or I/O memory
    if (vaddr >= USER_MEM_END && vaddr < IO_MEM_END) {
        return false;
    }

    // Check to see if we have a PDE for this page
    if (page_directory->phys_dir[GET_ADDR_PDE(vaddr)].present == 0) {
        // Allocate a kernel page for this PDE
        kvaddr_t new_table = kalloc_pages(1);

        if (new_table == NULL) {
            dprintf("Failed to allocate page table for identity mapping at vaddr=%p\n", vaddr);
            PANIC("Failed to allocate page table for identity mapping");
        }

        memset(new_table, 0, PAGE_SIZE);

        // Create a new PDE
        pde_t new_pde = {
            .present = 1,
            .readwrite = 1,
            .user = 1,
            .writethru = 0,
            .cachedisable = 0,
            .access = 0,
            .zero = 0,
            .size = 0,
            .ignore = 0,
            .address = STORE_ADDR(get_kphys(new_table))
        };

        set_page_directory_entry(page_directory, vaddr, (pte_t *)new_table, new_pde);
    }

    // Get the virtual address of the page tables for the vaddr
    pte_t *pt_entries = page_directory->virt_dir[GET_ADDR_PDE(vaddr)];
    pte_t *target_pte = &pt_entries[GET_ADDR_PTE(vaddr)];

    pte_t new_pte = {
        .present = 1,
        .readwrite = 1,
        .user = 0,
        .writethru = 0,
        .cached = 0,
        .access = 0,
        .dirty = 0,
        .zero = 0,
        .ignore = 0,
        .address = STORE_ADDR(paddr)
    };

    *target_pte = new_pte;

    invalidate_page(vaddr);

    return true;
}

void identity_map_pages(paddr_t vaddr, size_t count) {
    for (size_t i = 0; i < count; i++) {
        identity_map(vaddr + (i * PAGE_SIZE));
    }
}

void identity_unmap_page(paddr_t vaddr) {
    // Ensure we're not unmapping kernel or I/O memory
    if (vaddr >= USER_MEM_END && vaddr < IO_MEM_END) {
        dprintf("Failed to unmap identity page at vaddr=%p\n", vaddr);
        dprintf("  vaddr=%p is in kernel or I/O space!\n", vaddr);
        PANIC("Attempted to unmap identity page in kernel or I/O space");
    }

    if (page_directory->phys_dir[GET_ADDR_PDE(vaddr)].present == 0) {
        dprintf("Failed to unmap identity page at vaddr=%p\n", vaddr);
        dprintf("  PDE for vaddr=%p is not present!\n", vaddr);
        PANIC("Attempted to unmap identity page that is not mapped");
    }

    pte_t *pt_entries = page_directory->virt_dir[GET_ADDR_PDE(vaddr)];
    pte_t *target_entry = &pt_entries[GET_ADDR_PTE(vaddr)];
    
    if (target_entry->present == 0) {
        dprintf("Failed to unmap identity page at vaddr=%p\n", vaddr);
        dprintf("  PDE for vaddr=%p is present, but PTE is not!\n", vaddr);
        PANIC("Attempted to unmap identity page that is not mapped");
    }

    *target_entry = (pte_t) {0};

    // Reclaim the page directory entry if all pages within it are free
    bool all_free = true;
    for (size_t i = 0; i < PAGE_TABLE_ENTRY_COUNT; i++) {
        if (pt_entries[i].present) {
            all_free = false;
            break;
        }
    }

    if (all_free) {
        page_directory->phys_dir[GET_ADDR_PDE(vaddr)] = (pde_t) {0};
        kfree_pages(page_directory->virt_dir[GET_ADDR_PDE(vaddr)], 1);
        page_directory->virt_dir[GET_ADDR_PDE(vaddr)] = NULL;
    }

    invalidate_page(vaddr);
}

void identity_free_pages(paddr_t vaddr, size_t count) {
    for (size_t i = 0; i < count; i++) {
        identity_unmap_page(vaddr + (i * PAGE_SIZE));
    }
}

void dump_page_tables() {
    uint32_t cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    dprintf("CR3 register value: 0x%08X\n", cr3);

    dprintf("Page directory at vaddr=%p, paddr=%p:\n", page_directory, get_kphys((kvaddr_t)page_directory));
    for (size_t pd_index = 0; pd_index < PAGE_DIR_ENTRY_COUNT; pd_index++) {
        pde_t pde = page_directory->phys_dir[pd_index];
        if (pde.present) {
            dprintf("  PDE %03lX: vaddr=%p, paddr=%p\n", pd_index, pd_index * PDE_SIZE, GET_PDE_BASE_ADDR(pde));
        }
    }

    dprintf("End of page directory dump.\n");

    dprintf("Page tables:\n");
    for (size_t pd_index = 0; pd_index < PAGE_DIR_ENTRY_COUNT; pd_index++) {
        pde_t pde = page_directory->phys_dir[pd_index];
        if (pde.present) {
            pte_t *pt_entries = page_directory->virt_dir[pd_index];

            uint8_t perms = get_page_permissions(pt_entries);

            if (perms == PAGE_ERROR_NOT_PRESENT) {
                dprintf("  PDE %03lX: Unable to access page table entries\n", pd_index);
                continue;
            }

            for (size_t pt_index = 0; pt_index < PAGE_TABLE_ENTRY_COUNT; pt_index++) {
                pte_t pte = pt_entries[pt_index];
                if (pte.present) {
                    void *vaddr = (void *)(uintptr_t)((pd_index * PDE_SIZE) + (pt_index * PAGE_SIZE));
                    dprintf("  PTE %03lX.%03lX: vaddr=%p, paddr=%p\n", pd_index, pt_index, vaddr, GET_PTE_BASE_ADDR(pte));
                }
            }
        }
    }
    dprintf("End of page tables dump.\n");
}