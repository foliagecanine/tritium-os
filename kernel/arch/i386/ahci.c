#include <fs/disk.h>

typedef enum {
	RegHostToDevice,
	RegDeviceToHost,
	DMA_ACT,
	DMASetup,
	Data,
	BIST,
	PIOSetup,
	DEV_BIT
} FrameInfoType;

typedef struct {
	uint8_t fis_type;
	
	uint8_t portmul:4;
	uint8_t reserved0:3;
	uint8_t cmdAndCtrl:1;
	
	uint8_t cmd_reg;
	uint8_t feature_low;
	
	uint8_t lba0;
	uint8_t lba1;
	uint8_t lba2;
	uint8_t dev;
	
	uint8_t lba3;
	uint8_t lba4;
	uint8_t lba5;
	uint8_t feature_high;
	
	uint8_t count_low;
	uint8_t count_high;
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

	uint8_t  pmport:4;
	uint8_t  rsv0:1;
	uint8_t  d:1;
	uint8_t  i:1;
	uint8_t  a:1;

	uint8_t  reserved0;

	uint64_t DMA_buffer_id;

	uint32_t reserved1;

	uint32_t DMA_buffer_offset;

	uint32_t transfer_count;

	uint32_t reserved2;
 
} FIS_DMA_Setup;

void init_ahci() {
	kprint("Searching for SATA drives. Details below.");
	
	pci_t pci;
	
	for (uint16_t i = 0; i < 256; i++) {
		pci = getPCIData(i,0,0);
		if (pci.vendorID!=0xFFFF) {
			for (uint8_t j = 0; j < 32; j++) {
				pci = getPCIData(i,j,0);
				if (pci.vendorID!=0xFFFF&&pci.classCode==1&&pci.subclass==6) {
					printf("Detected SATA Drive on port %#.%#\n", (uint64_t)i,(uint64_t)j);
				}
			}
		}
	}
}