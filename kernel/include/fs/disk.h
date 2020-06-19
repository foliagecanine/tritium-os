#ifndef FS_DISK_H
#define FS_DISK_H

#include <kernel/stdio.h>
#include <kernel/pci.h>
#include <kernel/idt.h>
#include <kernel/mem.h>

#define HBA_DET_PRESENT 	3
#define HBA_IPM_ACTIVE 		1
#define SATA_READ_DMA_EX 	0x25
#define SATA_WRITE_DMA_EX   0x35
#define HBA_CMD_CR    		(1<<15)
#define HBA_CMD_FR    		(1<<14)
#define HBA_CMD_FRE   		(1<<4)
#define HBA_CMD_SUD   		(1<<1)
#define HBA_CMD_ST    		(1)
#define SATA_BUSY 			0x80
#define SATA_DRQ 			0x08

typedef volatile struct {
	uint32_t clb;
	uint32_t clbu;
	uint32_t fb;
	uint32_t fbu;
	uint32_t is;
	uint32_t ie;
	uint32_t cmd;
	uint32_t res0;
	uint32_t tfd;
	uint32_t sig;
	uint32_t ssts;
	uint32_t sctl;
	uint32_t serr;
	uint32_t sact;
	uint32_t ci;
	uint32_t sntf;
	uint32_t fbs;
	uint32_t res1[11];
	uint32_t vs[4];	
} HBAPort;

typedef volatile struct {
	uint32_t cap;
	uint32_t ghc;
	uint32_t is;
	uint32_t pi;
	uint32_t vs;
	uint32_t ccc_ctl;
	uint32_t ccc_ports;
	uint32_t em_loc;
	uint32_t em_ctl;
	uint32_t cap2;
	uint32_t bohc;
	
	uint8_t reserved[116];
	
	uint8_t vendorRegisters[96];
	
	HBAPort ports[1];
} HBAData;

typedef struct {
	uint8_t FIS_Type;
	
	uint8_t pmport:4;
	uint8_t reserved0:3;
	uint8_t c:1;
	
	uint8_t command;
	uint8_t feature_low;
	
	uint8_t lba0;
	uint8_t lba1;
	uint8_t lba2;
	uint8_t dev;
	
	uint8_t lba3;
	uint8_t lba4;
	uint8_t lba5;
	uint8_t feature_high;
	
	uint8_t countl;
	uint8_t counth;
	uint8_t isync_cmd_complete;
	uint8_t ctrl_reg;
	
	uint32_t reserved1;
} FIS_HostToDevice;

typedef struct {
	uint8_t fis_type;
	
	uint8_t portmul:4;
	uint8_t reserved0:2;
	uint8_t interrupt_bit:1;
	uint8_t reserved1:1;
	
	uint8_t status_reg;
	uint8_t error_reg;
	
	uint8_t lba0;
	uint8_t lba1;
	uint8_t lba2;
	uint8_t dev;
	
	uint8_t lba3;
	uint8_t lba4;
	uint8_t lba5;
	uint8_t reserved2;
	
	uint8_t count_low;
	uint8_t count_high;
	uint16_t reserved3;
	
	uint32_t reserved4;
} FIS_DeviceToHost;

typedef struct {
	uint8_t fis_type;
	
	uint8_t portmul:4;
	uint8_t reserved0:4;
	uint16_t reserved1;
	
	uint32_t data[1];
} FIS_Data;

typedef struct {
	uint8_t fis_type;
	
	uint8_t portmul:4;
	uint8_t reserved0:1;
	uint8_t data_direction:1;
	uint8_t interrupt_bit:1;
	uint8_t reserved1:1;
	
	uint8_t status_reg;
	uint8_t error_reg;
	
	uint8_t lba0;
	uint8_t lba1;
	uint8_t lba2;
	uint8_t dev;
	
	uint8_t lba3;
	uint8_t lba4;
	uint8_t lba5;
	uint8_t reserved2;
	
	uint8_t count_low;
	uint8_t count_high;
	uint8_t reserved3;
	uint8_t e_status;
	
	uint16_t transfer_count;
	uint16_t reserved4;
} FIS_PIO_Setup;

typedef struct {
	uint8_t  fis_type;

	uint8_t  portmul:4;
	uint8_t  reserved0:1;
	uint8_t  d:1;
	uint8_t  i:1;
	uint8_t  a:1;

	uint8_t  reserved1;

	uint64_t DMA_buffer_id;

	uint32_t reserved2;

	uint32_t DMA_buffer_offset;

	uint32_t transfer_count;

	uint32_t reserved3;
 
} FIS_DMA_Setup;

typedef volatile struct {
	FIS_DMA_Setup	DMASetup;
	uint8_t pad0[4];

	FIS_PIO_Setup	PIOSetup;
	uint8_t pad1[12];

	FIS_DeviceToHost	RegDeviceToHost;
	uint8_t pad2[4];

	uint16_t dev_bits;

	uint8_t ufis[64];

	uint8_t reserved[96];
} HBA_FIS;

typedef struct {
	uint8_t cfl:5;
	uint8_t a:1;
	uint8_t w:1;
	uint8_t p:1;
	uint8_t r:1;
	uint8_t b:1;
	uint8_t c:1;
	uint8_t reserved0:1;
	uint8_t pmp:4;
	
	uint16_t prdtl;
	
	volatile uint32_t prdbc;
	
	uint32_t ctba;
	uint32_t ctbau;
	
	uint32_t reserved1[4];
} HBACommandHeader;

typedef struct {
	uint32_t dba;
	uint32_t dbau;
	uint32_t reserved0;
 
	uint32_t dbc:22;
	uint32_t reserved1:9;
	uint32_t i:1;
} HBAPhysRegionDT;

typedef struct {
	uint8_t  cfis[64];
 
	uint8_t  acmd[16];
 
	uint8_t  res0[48];
 
	HBAPhysRegionDT prdt_entry[1];
} HBACommandTable;

void init_ahci();
uint8_t ahci_read_sector(uint8_t drive_num,uint64_t startSector,uint8_t *buf);
uint8_t ahci_read_sectors(uint8_t drive_num,uint64_t startSector,uint32_t count,uint8_t *buf);
uint8_t ahci_write_sector(uint8_t drive_num,uint64_t startSector,uint8_t *buf);
uint8_t ahci_write_sectors(uint8_t drive_num,uint64_t startSector,uint32_t count,uint8_t *buf);
_Bool drive_exists(uint8_t drive_num);
void print_sector(uint8_t *read);

#endif
