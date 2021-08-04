#include "elf.h"

typedef struct {
        char magic[4];
        uint8_t arch_class;
        uint8_t arch_endian;
        uint8_t file_version;
        uint8_t os_abi;
        uint8_t abi_version;
        uint8_t pad[7];
        uint16_t type;
        uint16_t arch_target;
        uint32_t elf_version;
        uint32_t entrypoint;
        uint32_t ph_offset;
        uint32_t sh_offset;
        uint32_t flags;
        uint16_t header_size;
        uint16_t ph_size;
        uint16_t ph_num;
        uint16_t sh_size;
        uint16_t sh_num;
        uint16_t sh_string_index;
} __attribute__((packed)) elfhdr32;

typedef struct {
        uint32_t type;
        uint32_t offset;
        uint32_t vaddr;
        uint32_t paddr;
        uint32_t filesize;
        uint32_t memsize;
        uint32_t flags;
        uint32_t align;
} __attribute__((packed)) elfph32;

elfhdr32 default_elf_header = {
	.magic = {0x7F, 'E', 'L', 'F'},
	.arch_class = 1,
	.arch_endian = 1,
	.file_version = 1,
	.os_abi = 0,
	.abi_version = 0,
	.pad = {0},
	.type = 2,
	.arch_target = 3,
	.elf_version = 1,
	.entrypoint = 0x8040000,
	.ph_offset = sizeof(elfhdr32),
	.sh_offset = 0,
	.flags = 0,
	.header_size = sizeof(elfhdr32),
	.ph_size = sizeof(elfph32),
	.ph_num = 1,
	.sh_size = 0,
	.sh_num = 0,
	.sh_string_index = 0
};

elfph32 default_program_header = {
	.type = 1,
	.offset = sizeof(elfhdr32)+sizeof(elfph32),
	.vaddr = 0x8040000,
	.paddr = 0,
	.filesize = 0, // ELF generator will fix this
	.memsize = 0, // ELF generator will fix this
	.flags = 7,
	.align = 0x1000
};

void *encode_elf(void *in, size_t len_in, size_t *len_out) {
	elfhdr32 new_elf_hdr;
	elfph32 new_program_hdr;
	memcpy(&new_elf_hdr,&default_elf_header,sizeof(elfhdr32));
	memcpy(&new_program_hdr,&default_program_header,sizeof(elfph32));
	new_program_hdr.filesize = len_in;
	new_program_hdr.memsize = len_in;
	void *elf_data = malloc(sizeof(elfhdr32)+sizeof(elfph32)+len_in);
	memcpy(elf_data,&new_elf_hdr,sizeof(elfhdr32));
	memcpy(elf_data+sizeof(elfhdr32),&new_program_hdr,sizeof(elfph32));
	memcpy(elf_data+sizeof(elfhdr32)+sizeof(elfph32),in,len_in);
	*len_out = sizeof(elfhdr32)+sizeof(elfph32)+len_in;
	return elf_data;
}