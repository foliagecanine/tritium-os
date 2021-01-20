#ifndef _KERNEL_ELF_H
#define _KERNEL_ELF_H

typedef struct {
	char magic[4];
	uint8_t arch_class;
	uint8_t arch_endian;
	uint8_t elf_version;
	uint8_t os_abi;
	uint8_t abi_version;
	uint8_t pad[7];
	uint16_t type;
	uint16_t arch_target;
	uint32_t elf_version2;
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

typedef struct {
	uint32_t name;
	uint32_t type;
	uint32_t flags;
	uint32_t addr;
	uint32_t offset;
	uint32_t size;
	uint32_t link;
	uint32_t info;
	uint32_t addralign;
	uint32_t entry_size;
} __attribute__((packed)) elfsh32;

#endif