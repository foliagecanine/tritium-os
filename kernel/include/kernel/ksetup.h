#ifndef _KERNEL_KSETUP_H
#define _KERNEL_KSETUP_H

#include <kernel/stdio.h>
#include <kernel/idt.h>
#include <kernel/pit.h>
#include <kernel/kbd.h>
#include <kernel/multiboot.h>
#include <kernel/tss.h>
#include <stdbool.h>
#include <kernel/acpi.h>

void init_gdt();
void serial_init();
void install_tss();
void init_paging(multiboot_info_t *mbi);
void identity_map(void *addr);
void map_addr(void *vaddr, void *paddr);
void unmap_vaddr(void *vaddr);
void* alloc_page(size_t pages);
void free_page(void *start, size_t pages);
void* calloc_page(size_t pages);
void *get_phys_addr(void *vaddr);
void *map_page_to(void *vaddr);
void mark_user(void *vaddr,_Bool user);
void mark_write(void *vaddr,_Bool write);
void *clone_tables();
void use_kernel_map();
void switch_tables(void *new);
uint32_t *get_current_tables();
void free_all_user_pages();
bool clone_user_pages();
void power_shutdown();
void power_reboot();
void kernel_exit();

typedef struct {
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
} __attribute__((packed)) page_dir_entry;

typedef struct {
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
} __attribute__((packed)) page_table_entry;

#endif
