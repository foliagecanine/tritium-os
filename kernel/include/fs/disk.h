#ifndef _FS_DISK_H
#define _FS_DISK_H

#include <kernel/stdio.h>
#include <kernel/idt.h>
#include <kernel/pci.h>

typedef struct {
	_Bool				exists;
	uint16_t 			iobase;
	unsigned char 	channel;
	unsigned char	type;
	unsigned char	signature;
	unsigned char	features;
	uint32_t			cmdsets;
	uint32_t 			size;
	char  				*model;
} ata_drive_t;

_Bool drive_exists(uint8_t drive_num);
ata_drive_t getATADrive(uint8_t drive_num);
uint8_t read_sectors_lba(uint8_t drive_num, uint32_t lba_start, uint8_t num_sectors, uint8_t *dest);
uint8_t write_sectors_lba(uint8_t drive_num, uint32_t lba_start, uint8_t num_sectors, uint8_t *dest);
uint8_t read_sector_lba(uint8_t drive_num, uint32_t lba, uint8_t *dest);
uint8_t write_sector_lba(uint8_t drive_num, uint32_t lba_start, uint8_t *dest);
void init_ata();
void init_ahci();

#endif
