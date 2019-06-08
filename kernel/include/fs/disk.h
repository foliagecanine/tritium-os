#ifndef _FS_DISK_H
#define _FS_DISK_H

#include <kernel/stdio.h>
#include <kernel/idt.h>

_Bool drive_exists(uint8_t drive_num);
uint8_t read_sectors_lba(uint8_t drive_num, uint32_t lba_start, uint8_t num_sectors, uint8_t *dest);
uint8_t write_sectors_lba(uint8_t drive_num, uint32_t lba_start, uint8_t num_sectors, uint8_t *dest);
uint8_t read_sector_lba(uint8_t drive_num, uint32_t lba, uint8_t *dest);
uint8_t write_sector_lba(uint8_t drive_num, uint32_t lba_start, uint8_t *dest);
void init_ata();

#endif