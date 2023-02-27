#include <kernel/mem.h>

/*
 * This is a rather simple malloc implementation (written by foliagecanine)
 * It is not particularly efficient with small allocations, but handles large ones fairly well.
 *
 * When an allocation is made, it attempts to make a new allocation in the remaining unused space.
 * If there is not enough space, it will simply allocate more memory than needed.
 *
 * In some cases, a zero-length allocation will be made in the remaining space.
 * The effect is no different than if no allocation were made EXCEPT if the allocation after the
 * zero-length one is freed, it can reclaim the space used by the freed allocation's alloc_t structure.
 *
 * The free function will attempt to reclaim the allocation after the freed allocation, as well as
 * before the freed allocation:
 * [Used][Unused][To be freed][Unused][Used] -> [Used][Unused][Used]
 * This should prevent any memory from being lost.
 *
 * The realloc function will try to merge the allocation with the current alloc or the alloc after the current alloc (if
 * it's free). If both of these fail, it will call malloc with the new size and copy the existing data.
 *
 * The calloc function just calls the malloc function and memsets it.
 *
 * TODO:
 * If the last allocation passes through a 4096 byte boundary and is freed, give the memory back to the kernel.
 *
 */

typedef struct alloc_t alloc_t;

struct alloc_t
{
    size_t   size;
    bool     used;
    alloc_t *next;
    alloc_t *prev;
    char     data[];
};

kheap_t current_heap;

kheap_t heap_create(uint32_t pages)
{
    kheap_t ret;
    void *  mem        = alloc_page(pages);
    ret.heap_start     = mem;
    ret.last_alloc_loc = ret.heap_start;
    ret.heap_end       = ret.heap_start;
    ret.heap_size      = pages;
    memset((char *)ret.heap_start, 0, ret.heap_end - ret.heap_start);
    alloc_t *root_alloc = ret.heap_start;
    root_alloc->used    = false;
    root_alloc->size    = pages * 4096;
    root_alloc->next    = NULL;
    root_alloc->prev    = NULL;
    return ret;
}

void heap_free(kheap_t heap)
{
    free_page((void *)heap.heap_start, heap.heap_size);
}

void set_current_heap(kheap_t heap)
{
    current_heap = heap;
}

kheap_t get_current_heap()
{
    return current_heap;
}

static bool grow_heap()
{
    current_heap.heap_end += 4096;
    if ((uintptr_t)(current_heap.heap_end - current_heap.heap_start) < current_heap.heap_size) return true;
    return false;
}

void *malloc(size_t size)
{
    alloc_t *current_alloc = current_heap.heap_start;
    while (true)
    {
        while (current_alloc->used)
        {
            current_alloc = current_alloc->next;
        }
        if (current_alloc->size == size)
        {
            // If it is the last alloc, we will need to create one after this.
            // Grow the heap and treat it as current_alloc->size > size.
            if (current_alloc == current_heap.last_alloc_loc)
            {
                if (!grow_heap()) return 0;
                current_alloc->size += 4096;
            }
            else
            {
                current_alloc->used = true;
                return (void *)current_alloc->data;
            }
        }
        if (current_alloc->size > size)
        {
            if (current_alloc->size < size + sizeof(alloc_t))
            {
                // Not enough space for a new alloc. Treat as if equal.
                if (current_alloc == current_heap.last_alloc_loc)
                {
                    grow_heap();
                    current_alloc->size += 4096;
                    // Assume sizeof(alloc_t) < 4096
                }
                else
                {
                    current_alloc->used = true;
                    return (void *)current_alloc->data;
                }
            }
            alloc_t *new_alloc = (void *)current_alloc->data + size;

            new_alloc->used = false;
            new_alloc->size = current_alloc->size - (sizeof(alloc_t) + size);
            new_alloc->next = current_alloc->next;
            new_alloc->prev = current_alloc;

            if (new_alloc->next == NULL) current_heap.last_alloc_loc = new_alloc;

            current_alloc->used = true;
            current_alloc->size = size;
            current_alloc->next = new_alloc;
            // prev is same

            return (void *)current_alloc->data;
        }
        if (current_alloc == current_heap.last_alloc_loc && current_alloc->size < size)
        {
            while (true)
            {
                if (!grow_heap()) return 0;
                current_alloc->size += 4096;
                if (current_alloc->size >= size) break;
            }
        }
    }
}

void free(void *ptr)
{
    alloc_t *current_alloc = ptr - sizeof(alloc_t);
    current_alloc->used    = false;

    alloc_t *next_alloc = current_alloc->next;
    alloc_t *prev_alloc = current_alloc->prev;

    if (next_alloc && !next_alloc->used)
    {
        current_alloc->next = next_alloc->next;
        current_alloc->size += next_alloc->size + sizeof(alloc_t);
        if (current_alloc->next == NULL) current_heap.last_alloc_loc = current_alloc;
    }

    if (prev_alloc && !prev_alloc->used)
    {
        prev_alloc->next = current_alloc->next;
        prev_alloc->size += current_alloc->size + sizeof(alloc_t);
        if (prev_alloc->next == NULL) current_heap.last_alloc_loc = prev_alloc;
    }
}