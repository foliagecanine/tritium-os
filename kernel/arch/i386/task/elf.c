#include <kernel/elf.h>

bool verify_elf(void *elf_file, size_t size) {
	elfhdr32 *elf_header = elf_file;
	
	if (elf_header->magic[0]!=0x7F ||
		elf_header->magic[1]!='E' ||
		elf_header->magic[2]!='L' ||
		elf_header->magic[3]!='F') {
		return false;
	}
	
	if (elf_header->type != ELF_TYPE_EXEC)
		return false;
		
	if (elf_header->ph_offset>size)
		return 0;
	
	elfph32 *program_headers = elf_file + elf_header->ph_offset;
		
	for (uint32_t i = 0; i < elf_header->ph_num; i++) {
		if (program_headers[i].offset>size)
			return false;
		if (program_headers[i].offset+program_headers[i].filesize>size)
			return false;
	}
		
	return true;
}

void load_elf_ph_section(elfph32 *ph, void *elf_file) {
	if (ph->type != ELF_PH_TYPE_LOAD)
		return;
	for (uint32_t i = 0; i < (ph->memsize+4095)/4096; i++)
		map_page_to((void *)(ph->vaddr+(i*4096)));
	memcpy((void *)ph->vaddr,elf_file+ph->offset,ph->filesize);
	for (uint32_t i = 0; i < (ph->memsize+4095)/4096; i++) {
		mark_user((void *)(ph->vaddr+(i*4096)), true);
		mark_write((void *)(ph->vaddr+(i*4096)), ph->flags&2);
	}
}

void *load_elf(void *elf_file) {
	elfhdr32 *elf_header = elf_file;
	
	elfph32 *program_headers = elf_file + elf_header->ph_offset;
	
	for (uint32_t i = 0; i < elf_header->ph_num; i++) {
		load_elf_ph_section(&program_headers[i], elf_file);
	}
	
	return (void *)(intptr_t)elf_header->entrypoint;
}