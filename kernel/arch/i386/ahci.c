#include <fs/disk.h>

/*
 * Based off of https://wiki.osdev.org/AHCI
 */

/*typedef enum {
	RegHostToDevice,
	RegDeviceToHost,
	DMA_ACT,
	DMASetup,
	Data,
	BIST,
	PIOSetup,
	DEV_BIT
} FrameInfoType;*/

typedef struct {
	uint8_t FIS_Type;
	
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
	uint32_t commandListBA;
	uint32_t commandListBA_u;
	uint32_t FIS_BA;
	uint32_t FIS_BA_u;
	uint32_t intStatus;
	uint32_t intEnabled;
	uint32_t cmd;
	uint32_t reserved0;
	uint32_t taskFileData;
	uint32_t signature;
	uint32_t SATA_Status;
	uint32_t SATA_Control;
	uint32_t SATA_Error;
	uint32_t SATA_Active;
	uint32_t commandIssue;
	uint32_t SATA_Notification;
	uint32_t FIS_SwitchControl;
	uint32_t reserved1[11];
	uint32_t vendor[4];	
} HBAPort;

typedef volatile struct {
	uint32_t capabilities;
	uint32_t globalHostControl;
	uint32_t interruptStatus;
	uint32_t portImplemented;
	uint32_t version;
	uint32_t ccc_control;
	uint32_t ccc_ports;
	uint32_t emLocation;
	uint32_t emControl;
	uint32_t capabilities2;
	uint32_t handoffControl;
	
	uint8_t reserved[116];
	
	uint8_t vendorRegisters[96];
	
	HBAPort ports[1];
} HBAData;

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
	uint8_t commandFISLength:5;
	uint8_t atapi:1;
	uint8_t write:1;
	uint8_t prefetchable:1;
	uint8_t reset:1;
	uint8_t BIST:1;
	uint8_t clearBusy:1;
	uint8_t reserved0:1;
	uint8_t portmul:4;
	
	uint16_t physRegionDescTableLength;
	
	volatile uint32_t physRegionDescByteCount;
	
	uint32_t cmdTableDescBA;
	uint32_t cmdTableDescBA_u;
	
	uint32_t reserved1[4];
} HBACommandHeader;

typedef struct {
	uint32_t dataBA;
	uint32_t dataBA_u;
	uint32_t reserved0;
 
	uint32_t dataByteCount:22;
	uint32_t reserved1:9;
	uint32_t intOnComplete:1;
} HBAPhysRegionDT;

typedef struct {
	uint8_t  commandFIS[64];
 
	uint8_t  ATAPI_Command[16];
 
	uint8_t  reserved[48];
 
	HBAPhysRegionDT physRegionDT_entry[1];	// Physical region descriptor table entries, 0 ~ 65535
} HBACommandTable;

void log_FH2D(FIS_HostToDevice *h2d) {
	printf("FIS_Type %d\n",(uint32_t) h2d->FIS_Type);
	
	printf("portmul %d\n",(uint32_t) h2d->portmul);
	printf("cmdAndCtrl %d\n",(uint32_t) h2d->cmdAndCtrl);
	
	printf("cmd_reg %d\n",(uint32_t) h2d->cmd_reg);
	printf("feature_low %d\n",(uint32_t) h2d->feature_low);
	
	printf("lba0 %d\n",(uint32_t) h2d->lba0);
	printf("lba1 %d\n",(uint32_t) h2d->lba1);
	printf("lba2 %d\n",(uint32_t) h2d->lba2);
	printf("dev %d\n",(uint32_t) h2d->dev);
	
	printf("lba3 %d\n",(uint32_t) h2d->lba3);
	printf("lba4 %d\n",(uint32_t) h2d->lba4);
	printf("lba5 %d\n",(uint32_t) h2d->lba5);
	printf("feature_high %d\n",(uint32_t) h2d->feature_high);
	
	printf("count_low %d\n",(uint32_t) h2d->count_low);
	printf("count_high %d\n",(uint32_t) h2d->count_high);
	printf("isync_cmd_complete %d\n",(uint32_t) h2d->isync_cmd_complete);
	printf("ctrl_reg %d\n",(uint32_t) h2d->ctrl_reg);
}

void log_HBAPort(HBAPort *h2d) {
	printf("commandListBA %d\n",(uint32_t) h2d->commandListBA);
	
	printf("commandListBA_u %d\n",(uint32_t) h2d->commandListBA_u);
	printf("FIS_BA %d\n",(uint32_t) h2d->FIS_BA);
	
	printf("FIS_BA_u %d\n",(uint32_t) h2d->FIS_BA_u);
	printf("intStatus %d\n",(uint32_t) h2d->intStatus);
	
	printf("intEnabled %d\n",(uint32_t) h2d->intEnabled);
	printf("cmd %d\n",(uint32_t) h2d->cmd);
	printf("taskFileData %d\n",(uint32_t) h2d->taskFileData);
	printf("signature %d\n",(uint32_t) h2d->signature);
	
	printf("SATA_Status %d\n",(uint32_t) h2d->SATA_Status);
	printf("SATA_Control %d\n",(uint32_t) h2d->SATA_Control);
	printf("SATA_Error %d\n",(uint32_t) h2d->SATA_Error);
	printf("SATA_Active %d\n",(uint32_t) h2d->SATA_Active);
	
	printf("commandIssue %d\n",(uint32_t) h2d->commandIssue);
	printf("SATA_Notification %d\n",(uint32_t) h2d->SATA_Notification);
	printf("FIS_SwitchControl %d\n",(uint32_t) h2d->FIS_SwitchControl);
}

HBAData **sata_data;
HBAPort **sata;
uint8_t numDrives = 0;

void initializePort(HBAPort *port, uint8_t portNum) {
	//Stop any current locks
	port->cmd &= ~0x0001;
	port->cmd &= ~0x0010;
	
	while(1) {
		if (port->cmd & 0x4000) {
			continue;
		}
		if (port->cmd & 0x8000) {
			continue;
		}
		break;
	}
	
	port->cmd &= ~0x0010;
	
	port->commandListBA = 0x400000;
	port->commandListBA_u = 0;
	memset((void *)port->commandListBA,0,1024);
	
	port->FIS_BA = 0x408000 + (portNum<<8);
	port->FIS_BA_u = 0;
	memset((void *)port->FIS_BA,0,256);
	
	HBACommandHeader *cmdHeader = (HBACommandHeader *)(port->commandListBA);
	for (uint8_t i = 0; i < 32; i++) {
		cmdHeader[i].physRegionDescTableLength = 8;
		cmdHeader[i].cmdTableDescBA = 0x40A000 + (portNum<<13) + (i<<8);
		cmdHeader[i].cmdTableDescBA_u = 0;
		memset((void *)cmdHeader[i].cmdTableDescBA,0,256);
	}
	
	port->SATA_Error = 0xFFFFFFFF;
	port->intStatus = 0xFFFFFFFF;
	
	//Start up our own lock
	while (port->cmd & 0x8000)
		;
	
	port->cmd |= 0x10;
	port->cmd |= 1;
}

void init_ahci_ports(HBAData *abar) {
	uint32_t portImplemented = abar->portImplemented;
	
	for (uint8_t l = 0; l < 32; l++) {
		
		if (portImplemented&1) {
			HBAPort *currentPort = &abar->ports[l];
			uint8_t det = currentPort->SATA_Status&0x0F;
			uint8_t ipm = (currentPort->SATA_Status>>8)&0x0F;
			if (det==3&&ipm==1) {
				printf("SATA drive found on port %d.\n",(uint32_t)l);
				numDrives++;
			}
		}
		
		portImplemented>>=1;
	}
	
	sata_data = (HBAData **)malloc(sizeof(HBAData)*numDrives);
	sata = (HBAPort **)malloc(sizeof(HBAPort)*numDrives);
	
	portImplemented = abar->portImplemented;
	
	for (uint8_t l = 0; l < 32; l++) {
		
		if (portImplemented&1) {
			HBAPort *currentPort = &abar->ports[l];
			uint8_t det = currentPort->SATA_Status&0x0F;
			uint8_t ipm = (currentPort->SATA_Status>>8)&0x0F;
			if (det==3&&ipm==1) {
				initializePort(&abar->ports[l],l);
				sata[l] = &abar->ports[l];
				sata_data = &abar;
			}
		}
		
		portImplemented>>=1;
	}
}

void printPCIData(pci_t pci, uint16_t i, uint8_t j, uint8_t k) {
	if (pci.vendorID!=0xFFFF&&pci.classCode==1&&pci.subclass==6) {
		if (k==0)
			printf("Detected SATA Host on port %#:%#\n", (uint64_t)i,(uint64_t)j);
		else
			printf("Detected SATA Host on port %#:%#.%d\n", (uint64_t)i,(uint64_t)j,(uint32_t)k);
		init_ahci_ports((HBAData *)pci.BAR5);
	}
}

void init_ahci() {
	kprint("Searching for SATA drives. Details below.");
	
	pci_t c_pci;
	for (uint16_t i = 0; i < 256; i++) {
		c_pci = getPCIData(i,0,0);
		if (c_pci.vendorID!=0xFFFF) {
			for (uint8_t j = 0; j < 32; j++) {
				c_pci = getPCIData(i,j,0);
				if (c_pci.vendorID!=0xFFFF) {
					printPCIData(c_pci,i,j,0);
					for (uint8_t k = 1; k < 8; k++) {
						pci_t pci = getPCIData(i,j,k);
						if (pci.vendorID!=0xFFFF) {
							printPCIData(pci,i,j,k);
						}
					}
				}
			}
		}
	}
}

int find_cmdslot(HBAPort *port,HBAData *abar) {
	uint32_t slots = (port->SATA_Active | port->commandIssue);
    uint16_t cmdslots= (abar->capabilities & 0x0f00)>>8 ;

    for (uint16_t i=0; i<cmdslots; i++) {
        if (!(slots&1))
            return (int)i;
        slots >>= 1;
    }
    return -1;
}

//This function is almost exactly as in wiki.osdev.org/AHCI, but the author dedicated it to CC0

uint8_t ahci_read_sectors(HBAPort *port, HBAData *abar, uint32_t startl, uint32_t starth, uint32_t numSectors, uint8_t *buffer) {
	uint16_t *buf = (uint16_t *)buffer;
	port->intStatus = 0xFFFFFFFE;
	int slot = find_cmdslot(port,abar);

	if (slot == -1)
		return 1;
	
	HBACommandHeader *cmdHeader = (HBACommandHeader *)port->commandListBA;
	cmdHeader+=slot;
	cmdHeader->commandFISLength = sizeof(FIS_HostToDevice)/sizeof(uint32_t);
	cmdHeader->write = 0;
	cmdHeader->physRegionDescTableLength = (uint16_t)((numSectors-1)>>4)+1;
	
	HBACommandTable *cmdTable = (HBACommandTable *)(cmdHeader->cmdTableDescBA);
	memset(cmdTable,0,sizeof(HBACommandTable)+(cmdHeader->physRegionDescTableLength-1)*sizeof(HBAPhysRegionDT));
	
    /*// 8K bytes (16 sectors) per PRDT
    for (int i=0; i<cmdHeader->physRegionDescTableLength-1; i++)
    {
        cmdTable->physRegionDT_entry[i].dataBA = (uint32_t)buf;
        cmdTable->physRegionDT_entry[i].dataByteCount = 8*1024-1; // 8K bytes
        cmdTable->physRegionDT_entry[i].intOnComplete = 1;
        buf += 4*1024;  // 4K words
        numSectors -= 16;    // 16 sectors
    }*/
	
	cmdTable->physRegionDT_entry[0].dataBA = (uint32_t)buf;
	cmdTable->physRegionDT_entry[0].dataByteCount = (numSectors<<9)-1;
	cmdTable->physRegionDT_entry[0].intOnComplete = 1;
	
	FIS_HostToDevice *cmdFIS = (FIS_HostToDevice *)(&cmdTable->commandFIS);
	
	cmdFIS->FIS_Type = 0x27;
	cmdFIS->cmdAndCtrl = 1;
	cmdFIS->cmd_reg = 0x25;
	
	cmdFIS->lba0 = (uint8_t)startl;
	cmdFIS->lba1 = (uint8_t)(startl>>8);
	cmdFIS->lba2 = (uint8_t)(startl>>16);
	cmdFIS->dev = 1<<6;
	
	cmdFIS->lba3 = (uint8_t)(startl>>24);
	cmdFIS->lba4 = (uint8_t)starth;
	cmdFIS->lba5 = (uint8_t)(starth>>8);
	
	cmdFIS->count_low = numSectors & 0xFF;
	cmdFIS->count_high = (numSectors >> 8) & 0xFF;

	uint32_t timeout = 0;
	
	while ((port->taskFileData & (0x88)) && timeout < 1000000) {
		timeout++;
	}
	
	if (timeout==1000000) {
		return 2;
	}
	
	port->commandIssue = 1<<slot;
	
	while (true) {
		if (!(port->commandIssue & (1<<slot)))
			break;
		if (port->intStatus & (1<<30)) {
			return 3;
		}
	}
	
	if (port->intStatus & (1<<30)) {
		return 3;
	}
	
	sleep(100);
	
	return 0;
}

_Bool ahci_read_sector(uint8_t drive_num, uint64_t startSector, uint8_t *buffer) {
	return ahci_read_sectors(sata[drive_num-8],sata_data[drive_num-8],startSector&0xFFFFFFFF,(startSector>>32)&0xFFFFFFFF,1,buffer);
}

void ahci_read_test() {
	uint8_t read[512];
	for (uint32_t i = 0; i < 2880; i++) {
		memset(read,0,512);
		ahci_read_sector(8,i,read);
		for (int j = 0; j <16; j++) {
			for (int i = 0; i<32; i++) {
				if (read[(j*32)+i]<16) {
					printf("0");
				}
				printf("%#", (uint64_t)(read[(j*32)+i]));
			}
			printf("\n");
		}
		sleep(1000);
		char key = getchar();
		if (key == ' ')
			break;
	}
}
