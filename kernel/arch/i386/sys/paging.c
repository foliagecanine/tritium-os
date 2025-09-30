#include <kernel/ksetup.h>
#include <kernel/mem.h>
#include <kernel/syscalls.h>
#include <kernel/sysfunc.h>

#define PAGE_SIZE 4096
#define PAGEDIR_ADDR 768
#define KERNEL_ADDR 0xC0000000
#define PAGETABLE_SIZE 0x00400000
#define PAGETABLE_ADDR 0xC0400000

page_dir_entry   kernel_page_dir[1024] __attribute__((aligned(PAGE_SIZE)));
page_table_entry new_base_page_table[1024] __attribute__((aligned(PAGE_SIZE)));
page_table_entry full_kernel_table_storage[1024] __attribute__((aligned(PAGE_SIZE)));

page_table_entry *kernel_tables = (page_table_entry *)PAGETABLE_ADDR;

// We'll also have a PMEM manager in this file too. This table will determine if a physical page is being used or not.
uint8_t     pmem_used[131072] __attribute__((aligned(PAGE_SIZE)));
const char *mem_types[] = {"ERROR", "Available", "Reserved", "ACPI Reclaimable", "NVS", "Bad RAM"};

multiboot_memory_map_t *mmap;

_Bool check_phys_page(void *addr)
{
    uint32_t page = (uint32_t)addr;
    page /= PAGE_SIZE;
    _Bool r = (_Bool)(pmem_used[page / 8] & (1 << (page % 8)));
    return r;
}

bool warn_claimed = true;

void claim_phys_page(void *addr)
{
    // FOR DEBUGGING
    if (check_phys_page(addr) && warn_claimed)
    {
        dprintf("[WARN] Attempting to claim already claimed page: %p\n", addr);
    }
    uint32_t page = (uint32_t)addr;
    page /= PAGE_SIZE;
    pmem_used[page / 8] |= 1 << (page % 8);
}

void release_phys_page(void *addr)
{
    uint32_t page = (uint32_t)addr;
    page /= PAGE_SIZE;
    pmem_used[page / 8] &= ~(1 << (page % 8));
}

uint8_t check_page_cluster(void *addr)
{
    uint32_t page = (uint32_t)addr;
    page /= PAGE_SIZE;
    return pmem_used[page / 8];
}

inline page_table_entry get_table_vaddr(void *vaddr)
{
    return kernel_tables[(uint32_t)vaddr / PAGE_SIZE];
}

inline void flush_tlb() {
    asm volatile("movl %%cr3, %%ecx; movl %%ecx, %%cr3" ::: "ecx", "memory");
}

inline void update_cr3(uint32_t new_cr3) {
    asm volatile("mov %0, %%cr3" ::"r"(new_cr3) : "memory");
}

void init_paging(multiboot_info_t *mbi)
{
    // Stage 1:
    // First, we need to identity map a new portion of RAM so that we have enough space for a new paging table
    // We don't have any programs we're going to break, so any ram around us *should* be fine.
    kernel_page_dir[PAGEDIR_ADDR].present   = 1;
    kernel_page_dir[PAGEDIR_ADDR].readwrite = 1;
    uint32_t nbpt_addr                      = (uint32_t)&new_base_page_table[0];
    nbpt_addr -= KERNEL_ADDR;
    nbpt_addr >>= 12;
    kernel_page_dir[PAGEDIR_ADDR].address = nbpt_addr;
    for (uint16_t i = 0; i < 1023; i++)
    {
        if ((i * PAGE_SIZE) >= 0x100000)
        {
            new_base_page_table[i].present   = 1;
            new_base_page_table[i].readwrite = 1;
            new_base_page_table[i].address   = i;
        }
    }
    new_base_page_table[1023].present   = 1;
    new_base_page_table[1023].readwrite = 1;
    new_base_page_table[1023].address   = 0x000B8;

    uint32_t new_addr = (uint32_t)&kernel_page_dir[0];
    new_addr -= KERNEL_ADDR;
    update_cr3(new_addr);

    // Stage 2:
    // Next, we allocate 4MiB of space for our new FULL page table.

    kernel_page_dir[PAGEDIR_ADDR + 1].present   = 1;
    kernel_page_dir[PAGEDIR_ADDR + 1].readwrite = 1;
    kernel_page_dir[PAGEDIR_ADDR + 1].address   = (((uint32_t)&full_kernel_table_storage) - KERNEL_ADDR) >> 12;

    for (uint16_t i = 0; i < 1024; i++)
    {
        full_kernel_table_storage[i].present   = 1;
        full_kernel_table_storage[i].readwrite = 1;
        full_kernel_table_storage[i].address   = (PAGETABLE_SIZE >> 12) + i;
    }

    // Reset the CR3 so it flushes the TLB
    flush_tlb();

    // Stage 3:
    // Create a new full page table so we can allocate things whenever we want

    // First let's do a memset so we dont have any random data
    memset((void *)PAGETABLE_ADDR, 0, PAGETABLE_SIZE);

    for (uint16_t i = 0; i < 1023; i++)
    {
        if ((i * PAGE_SIZE) >= 0x100000)
        {
            kernel_tables[(768 * 1024) + i].present   = 1;
            kernel_tables[(768 * 1024) + i].readwrite = 1;
            kernel_tables[(768 * 1024) + i].address   = i;
        }
    }

    kernel_tables[(768 * 1024) + 1023].present   = 1;
    kernel_tables[(768 * 1024) + 1023].readwrite = 1;
    kernel_tables[(768 * 1024) + 1023].address   = 0x000B8;

    for (uint16_t i = 0; i < 1024; i++)
    {
        kernel_tables[(769 * 1024) + i].present   = 1;
        kernel_tables[(769 * 1024) + i].readwrite = 1;
        kernel_tables[(769 * 1024) + i].address   = (0x400000 >> 12) + i;
    }

    // Now we link the tables to the page directory, whose location won't change
    for (uint16_t i = 0; i < 1024; i++)
    {
        kernel_page_dir[i].address   = 0x400 + (i);
        kernel_page_dir[i].readwrite = 1;
        kernel_page_dir[i].present   = 1;
        kernel_page_dir[i].user      = 1; // All page directory entries to user because we can set it to supervisor only
                                          // inside of the page table entry
    }

    // Reset the CR3 so it flushes the TLB
    flush_tlb();

    // Awesome! Now the default page tables are at vaddr:0xC0400000 and paddr:0x400000
    // This won't get in the way of programs (which are loaded at vaddr:0x100000) but is still within a low capacity of
    // memory (we only need 8MiB of memory right now)
    kprint("[INIT] Paging initialized");

    // Allow us to read the MBI to find the memory map.
    identity_map(mbi);
    mmap = (multiboot_memory_map_t *)mbi->mmap_addr;

    // Clear out the current map so all entries are "claimed"
    memset((void *)&pmem_used[0], 255, 131072);

    // Scan the memory map for areas that we can use for general purposes
    for (uint8_t i = 0; i < 15; i++)
    {
        uint32_t type = mmap[i].type;
        if (mmap[i].type > 5) type = 2;
        if (i > 0 && mmap[i].addr == 0) break;
        if (mmap[i].type == 1)
        {
            for (uint64_t physptr = mmap[i].addr; physptr < mmap[i].addr + mmap[i].len; physptr += PAGE_SIZE)
            {
                release_phys_page((void *)(uint32_t)physptr);
            }
        }
        dprintf("%hhu: %010p+%010p %s\n", i, (void *)(uintptr_t)mmap[i].addr, (void *)(uintptr_t)mmap[i].len,
                mem_types[type]);
        printf("%hhu: %010p+%010p %s\n", i, (void *)(uintptr_t)mmap[i].addr, (void *)(uintptr_t)mmap[i].len,
               mem_types[type]);
    }

    // Reclaim all the way up to 8MiB (for the kernel and the page tables)
    memset((void *)&pmem_used[0], 255, 256);

    uint32_t cr3 = 0;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));

    kprint("[INIT] Kernel heap initialized");
}

void identity_map(void *addr)
{
    uint32_t page = (uint32_t)addr;
    page /= PAGE_SIZE;
    // Don't do anything if already mapped.
    if (kernel_tables[page].present && kernel_tables[page].readwrite &&
        kernel_tables[page].address == (uint32_t)addr >> 12)
        return;
    warn_claimed = false;
    claim_phys_page(addr);
    warn_claimed = true;
    /* volatile uint32_t directory_entry = page/1024;
    volatile uint32_t table_entry = page % 1024; */
    kernel_tables[page /* (directory_entry*1024)+table_entry */].address   = page;
    kernel_tables[page /* (directory_entry*1024)+table_entry */].readwrite = 1;
    kernel_tables[page /* (directory_entry*1024)+table_entry */].present   = 1;
    flush_tlb();
}

void map_addr(void *vaddr, void *paddr)
{
    claim_phys_page(paddr);
    uint32_t vaddr_page = (uint32_t)vaddr;
    vaddr_page /= PAGE_SIZE;
    uint32_t paddr_page = (uint32_t)paddr;
    paddr_page /= PAGE_SIZE;
    kernel_tables[vaddr_page].address   = paddr_page;
    kernel_tables[vaddr_page].readwrite = 1;
    kernel_tables[vaddr_page].present   = 1;
    flush_tlb();
}

void unmap_vaddr(void *vaddr)
{
    uint32_t vaddr_page = (uint32_t)vaddr;
    vaddr_page /= PAGE_SIZE;
    release_phys_page((void *)(kernel_tables[vaddr_page].address * PAGE_SIZE));
    kernel_tables[vaddr_page].address   = 0;
    kernel_tables[vaddr_page].readwrite = 0;
    kernel_tables[vaddr_page].present   = 0;
    flush_tlb();
}

void trade_vaddr(void *vaddr)
{
    uint32_t vaddr_page = (uint32_t)vaddr;
    vaddr_page /= PAGE_SIZE;
    // Don't release the page, because the program has given it to another address space
    kernel_tables[vaddr_page].address   = 0;
    kernel_tables[vaddr_page].readwrite = 0;
    kernel_tables[vaddr_page].present   = 0;
    flush_tlb();
}

void mark_user(void *vaddr, _Bool user)
{
    kernel_tables[(uint32_t)vaddr / PAGE_SIZE].user = user ? 1 : 0;
    flush_tlb();
}

void mark_write(void *vaddr, _Bool write)
{
    kernel_tables[(uint32_t)vaddr / PAGE_SIZE].readwrite = write ? 1 : 0;
    flush_tlb();
}

void *get_phys_addr(void *vaddr)
{
    return (void *)((kernel_tables[(uint32_t)vaddr / PAGE_SIZE].address * PAGE_SIZE) + ((uint32_t)vaddr & 0xFFF));
}

void *find_free_phys_page()
{
    for (uint32_t k = 0x800; k < 1048576; k++)
    {
        if (check_page_cluster((void *)(k * PAGE_SIZE)) == 255)
            k += 7;
        else if (!check_phys_page((void *)(k * PAGE_SIZE)))
        {
            return (void *)(uintptr_t)(k * PAGE_SIZE);
        }
    }
    dprintf("failed\n");
    return 0;
}

void *map_page_to(void *vaddr)
{
    if (kernel_tables[(uint32_t)vaddr / PAGE_SIZE].present)
    {
        kwarn("[WARN] Page map attempted on page that is already present");
        printf("===== %p\n", vaddr);
        return 0;
    }
    void *paddr = find_free_phys_page();
    if (!paddr)
    {
        PANIC("OUT OF MEMORY");
        return 0;
    }
    else
    {
        map_addr(vaddr, paddr);
        return vaddr;
    }
}

void map_page_secretly(void *vaddr, void *paddr)
{
    uint32_t vaddr_page = (uint32_t)vaddr;
    vaddr_page /= PAGE_SIZE;
    uint32_t paddr_page = (uint32_t)paddr;
    paddr_page /= PAGE_SIZE;
    kernel_tables[vaddr_page].address   = paddr_page;
    kernel_tables[vaddr_page].readwrite = 1;
    kernel_tables[vaddr_page].present   = 1;
    flush_tlb();
}

void unmap_secret_page(void *vaddr, size_t pages)
{
    uint32_t vaddr_page = (uint32_t)vaddr;
    vaddr_page /= PAGE_SIZE;
    for (size_t i = 0; i < pages; i++)
    {
        kernel_tables[vaddr_page + i].address   = 0;
        kernel_tables[vaddr_page + i].readwrite = 0;
        kernel_tables[vaddr_page + i].present   = 0;
    }
    flush_tlb();
}

void *map_paddr(void *paddr, size_t pages)
{
    if (!pages) return 0;
    uint32_t paddr_page = (uint32_t)paddr;
    paddr_page /= PAGE_SIZE;
    for (uint32_t i = 786432; i < 1048576; i++)
    {
        if (!kernel_tables[i].present)
        {
            size_t successful_pages = 1;
            for (uint32_t j = i + 1; j - i < pages + 1; j++)
            {
                if (successful_pages == pages) break;
                if (!kernel_tables[j].present)
                {
                    successful_pages++;
                }
                else
                    break;
            }
            if (successful_pages == pages)
            {
                for (uint32_t step = 0; step < pages; step++)
                {
                    map_page_secretly((void *)(uintptr_t)((i + step) * PAGE_SIZE),
                                      (void *)(uintptr_t)((paddr_page + step) * PAGE_SIZE));
                }
                return (void *)(uintptr_t)(i * PAGE_SIZE) + ((uintptr_t)paddr & 0xFFF);
            }
        }
    }
    PANIC("OUT OF MEMORY");
    return 0;
}

void *alloc_page(size_t pages)
{
    if (!pages) return 0;
    // First find consecutive virtual pages in kernel memory.
    for (uint32_t i = 786432; i < 1048576; i++)
    {
        if (!kernel_tables[i].present)
        {
            size_t successful_pages = 1;
            for (uint32_t j = i + 1; j - i < pages + 1; j++)
            {
                if (successful_pages == pages) break;
                if (!kernel_tables[j].present)
                {
                    successful_pages++;
                }
                else
                    break;
            }
            if (successful_pages == pages)
            {
                for (uint32_t step = 0; step < pages; step++)
                {
                    void *paddr = find_free_phys_page();
                    if (!paddr)
                    {
                        PANIC("OUT OF MEMORY");
                        return 0;
                    }
                    map_addr((void *)((i + step) * PAGE_SIZE), paddr);
                }
                return (void *)(i * PAGE_SIZE);
            }
        }
    }
    PANIC("OUT OF MEMORY");
    return 0;
}

void free_page(void *start, size_t pages)
{
    for (uint32_t i = 0; i < pages; i++)
    {
        unmap_vaddr((void *)((uint32_t)start + (i * PAGE_SIZE)));
    }
}

void *calloc_page(size_t pages)
{
    void *out = alloc_page(pages);
    memset(out, 0, PAGE_SIZE * pages);
    return out;
}

// Similar to alloc_page, but requires physical pages to be sequential
void *alloc_sequential(size_t pages)
{
    if (!pages) return 0;
    for (uint32_t i = 786432; i < 1048576; i++)
    {
        if (!kernel_tables[i].present)
        {
            size_t successful_pages = 1;
            for (uint32_t j = i + 1; j - i < pages + 1; j++)
            {
                if (successful_pages == pages) break;
                if (!kernel_tables[j].present)
                {
                    successful_pages++;
                }
                else
                    break;
            }
            if (successful_pages == pages)
            {
                for (uint32_t j = PAGE_SIZE; j < 1048576; j++)
                {
                    successful_pages = 1;
                    if (check_page_cluster((void *)(j * PAGE_SIZE)) == 255)
                        j += 7;
                    else if (!check_phys_page((void *)(j * PAGE_SIZE)))
                    {
                        for (uint32_t k = j + 1; k - j < pages + 1; k++)
                        {
                            if (successful_pages == pages) break;
                            if (!check_phys_page((void *)(k * PAGE_SIZE)))
                            {
                                successful_pages++;
                            }
                            else
                                break;
                        }
                        if (successful_pages == pages)
                        {
                            for (uint32_t k = 0; k < pages; k++)
                            {
                                map_addr((void *)(uintptr_t)((i + k) * PAGE_SIZE), (void *)(uintptr_t)((j + k) * PAGE_SIZE));
                            }
                            return (void *)(i * PAGE_SIZE);
                        }
                    }
                }
            }
        }
    }
    PANIC("OUT OF MEMORY");
    return 0;
}

uint32_t *get_current_tables()
{
    return (uint32_t *)kernel_tables;
}

void switch_tables(void *new)
{
    kernel_tables = new;
}

void use_kernel_map()
{
    kernel_tables = (page_table_entry *)PAGETABLE_ADDR;
    update_cr3((uint32_t)(uintptr_t)(&kernel_page_dir[0]) - KERNEL_ADDR);
}

void *clone_tables()
{
    void *new = alloc_sequential(1025);
    memset(new, 0, PAGE_SIZE * 1025);
    page_table_entry *new_page_tables = new + PAGE_SIZE;
    page_dir_entry *  new_page_dir    = new;
    memcpy(new_page_tables, kernel_tables, 1024 * PAGE_SIZE);
    memset(new_page_dir, 0, PAGE_SIZE);
    for (uint32_t i = 0; i < 1024; i++)
    {
        page_dir_entry *pde = &new_page_dir[i];
        pde->user           = 1;
        pde->present        = 1;
        pde->readwrite      = 1;
        pde->address        = ((uint32_t)get_phys_addr(new + PAGE_SIZE) / 0x1000) + i;
    }

    uint32_t new_addr = (uint32_t)get_phys_addr(new);
    switch_tables(new + PAGE_SIZE);
    update_cr3(new_addr);

    return (void *)new_addr;
}

uint32_t free_pages()
{
    uint32_t retval = 0;
    for (uint32_t i = 0; i < 131072; i++)
    {
        if (!pmem_used[i])
            retval += 8;
        else if (pmem_used[i] != (uint8_t)255)
        {
            for (uint8_t j = 0; j < 8; j++)
            {
                if (!check_phys_page((void *)(i * 8 * PAGE_SIZE) + (j * PAGE_SIZE))) retval++;
            }
        }
    }
    return retval;
}

void *realloc_page(void *ptr, uint32_t old_pages, uint32_t new_pages)
{
    int64_t  amt_pages = (int64_t)new_pages - (int64_t)old_pages;
    uint32_t ptr_page  = (uint32_t)ptr / PAGE_SIZE;
    if (old_pages == new_pages)
        return ptr;
    else if (old_pages < new_pages)
    {
        bool failed = false;
        for (uint32_t i = 0; i < (uint32_t)amt_pages; i++)
        {
            if (kernel_tables[ptr_page + old_pages + i].present)
            {
                failed = true;
                break;
            }
        }
        if (failed)
        {
            void *ret = alloc_page(new_pages);
            memcpy(ret, ptr, old_pages * PAGE_SIZE);
            free_page(ptr, old_pages);
            return ret;
        }
        else
        {
            for (uint32_t i = 0; i < (uint32_t)amt_pages; i++)
            {
                map_page_to(ptr + (old_pages * PAGE_SIZE) + (i * PAGE_SIZE));
            }
            return ptr;
        }
    }
    else
    {
        for (int64_t i = 0; i < -amt_pages; i++)
        {
            unmap_vaddr(ptr + (new_pages * PAGE_SIZE) + (i * PAGE_SIZE));
        }
        return ptr;
    }
}

void free_all_user_pages()
{
    for (uint32_t i = 0; i < 1024 * 1024; i++)
    {
        if (kernel_tables[i].user && kernel_tables[i].present)
        {
            free_page((void *)(uintptr_t)(i * PAGE_SIZE), 1);
        }
    }
}

uint8_t clone_data[PAGE_SIZE];

bool clone_user_pages()
{
    for (uint32_t i = 0; i < 1024 * 1024; i++)
    {
        if (kernel_tables[i].user && kernel_tables[i].present)
        {
            // Find free physical page
            void *paddr = find_free_phys_page();
            if (!paddr)
            {
                PANIC("OUT OF MEMORY");
                return false;
            }

            // Copy the data from the original page
            memcpy(clone_data, (void *)(i * PAGE_SIZE), PAGE_SIZE);

            // Swap the virtual address' physical page for the new physical page
            kernel_tables[i].address = (uint32_t)paddr >> 12;
            claim_phys_page(paddr);

            // Set page to writeable when we are copying it
            uint8_t rw                 = kernel_tables[i].readwrite;
            kernel_tables[i].readwrite = true;

            // Flush TLB to update the map
            flush_tlb();

            // Copy the data into the new page
            memcpy((void *)(i * PAGE_SIZE), clone_data, PAGE_SIZE);

            // Set page's write permission back to what it was before.
            // This will be updated next time we flush the TLB.
            kernel_tables[i].readwrite = rw;
        }
    }
    // Fix last changed page's write permission
    flush_tlb();
    return true;
}

bool check_user(void *vaddr)
{
    uint32_t vaddr_page = (uint32_t)vaddr;
    vaddr_page /= PAGE_SIZE;
    return kernel_tables[vaddr_page].user;
}

uint8_t get_page_permissions(void *vaddr)
{
    uint32_t vaddr_page = (uint32_t)vaddr;
    vaddr_page /= PAGE_SIZE;
    return kernel_tables[vaddr_page].present | (kernel_tables[vaddr_page].readwrite << 1) |
           (kernel_tables[vaddr_page].user << 2);
}
