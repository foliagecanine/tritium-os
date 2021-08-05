#ifndef FS_DISK_H
#define FS_DISK_H

#include <fs/ahci.h>
#include <fs/ide.h>
#include <kernel/stdio.h>

uint8_t register_disk_handler(uint8_t (*read_function)(uint16_t, uint64_t, uint32_t, void *), uint8_t (*write_function)(uint16_t, uint64_t, uint32_t, void *));
void    unregister_disk_handler(uint8_t handler_id);
uint8_t disk_read_sectors(uint32_t drive_num, uint64_t start_sector, uint32_t count, void *buf);
uint8_t disk_write_sectors(uint32_t drive_num, uint64_t start_sector, uint32_t count, void *buf);

#endif
