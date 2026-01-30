#include <kernel/elf.h>

bool verify_elf(void *elf_file, size_t size)
{
    elfhdr32 *elf_header = elf_file;

    if (elf_header->magic[0] != 0x7F || elf_header->magic[1] != 'E' || elf_header->magic[2] != 'L' ||
        elf_header->magic[3] != 'F')
    {
        return false;
    }

    if (elf_header->type != ELF_TYPE_EXEC)
        return false;

    if (elf_header->ph_offset > size)
        return 0;

    elfph32 *program_headers = elf_file + elf_header->ph_offset;

    for (uint32_t i = 0; i < elf_header->ph_num; i++)
    {
        if (program_headers[i].offset > size)
            return false;
        if (program_headers[i].offset + program_headers[i].filesize > size)
            return false;
    }

    return true;
}

void load_elf_ph_section(elfph32 *ph, void *elf_file)
{
    if (ph->type != ELF_PH_TYPE_LOAD)
        return;

    size_t ph_pages = (ph->memsize + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint32_t i = 0; i < ph_pages; i++) {
        map_user_page((void *)(ph->vaddr + (i * PAGE_SIZE)));
    }

    memcpy((void *)ph->vaddr, elf_file + ph->offset, ph->filesize);

    for (uint32_t i = 0; i < ph_pages; i++) {
        mark_write((void *)(ph->vaddr + (i * PAGE_SIZE)), ph->flags & ELF_PH_FLAG_WRITE);
    }
}

void *load_elf(void *elf_file)
{
    elfhdr32 *elf_header = elf_file;

    elfph32 *program_headers = elf_file + elf_header->ph_offset;

    for (uint32_t i = 0; i < elf_header->ph_num; i++)
    {
        load_elf_ph_section(&program_headers[i], elf_file);
    }

    return (void *)(intptr_t)elf_header->entrypoint;
}