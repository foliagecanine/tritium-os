#ifndef FS_DISK_H
#define FS_DISK_H

#include <kernel/stdio.h>
#include <kernel/pci.h>
#include <kernel/idt.h>
#include <kernel/mem.h>

void init_ahci();
_Bool ahci_read_sector(uint8_t drive_num,uint64_t startSector,uint8_t *);
void ahci_read_test();

#endif
