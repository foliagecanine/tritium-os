#include <fs/disk.h>

/* Error codes:
 *
 * 1 : No available slot
 * 2 : Port hung
 * 3 : Disk error
 * 4 : Drive not found
 */

uint8_t driver_id;

typedef struct {
	HBAData *abar;
	HBAPort *port;
	void *   clb;
	void *   fb;
	void *   ctba[32];
	void *   unused[28]; // Even out the data size to 256 bytes
} ahci_port;

ahci_port *ports;
uint32_t   num_ports;

uint32_t find_cmdslot(ahci_port aport) {
	HBAPort *port = aport.port;
	uint32_t slots = (port->sact | port->ci);
	uint32_t cmdslots = (aport.abar->cap & 0x0f00) >> 8;
	for (uint32_t i = 0; i < cmdslots; i++) {
		if ((slots & 1) == 0)
			return i;
		slots >>= 1;
	}
	return 0xFFFFFFFF;
}

uint8_t ahci_identify_device(ahci_port aport, void *buf) {
	HBAPort *port = aport.port;
	port->is = 0xFFFFFFFF;
	uint32_t slot = find_cmdslot(aport);
	if (slot == 0xFFFFFFFF)
		return 1;

	HBACommandHeader *cmdheader = (HBACommandHeader *)aport.clb;
	cmdheader += slot;
	cmdheader->cfl = sizeof(FIS_HostToDevice) / sizeof(uint32_t);
	cmdheader->w = 0;

	HBACommandTable *cmdtable = (HBACommandTable *)aport.ctba[slot];

	cmdtable->prdt_entry[0].dba = (uint32_t)(intptr_t)get_phys_addr(buf);
	cmdtable->prdt_entry[0].dbau = 0;
	cmdtable->prdt_entry[0].dbc = 511;
	cmdtable->prdt_entry[0].i = 1;

	FIS_HostToDevice *cmdfis = (FIS_HostToDevice *)(&cmdtable->cfis);

	cmdfis->FIS_Type = 0x27; // Host to device
	cmdfis->c = 1;
	cmdfis->command = SATA_IDENTIFY_DEVICE;

	cmdfis->dev = 1 << 6;

	for (uint32_t spin = 0; spin < 1000000; spin++) {
		if (!(port->tfd & (SATA_BUSY | SATA_DRQ)))
			break;
	}
	if ((port->tfd & (SATA_BUSY | SATA_DRQ)))
		return 2;

	port->ci = (1 << slot);

	while (1) {
		if (!(port->ci & (1 << slot)))
			break;
		if (port->is & (1 << 30))
			return 3;
	}

	if (port->is & (1 << 30))
		return 3;

	return 0;
}

uint8_t ahci_read_sectors_internal(ahci_port aport, uint32_t startl, uint32_t starth, uint32_t count, uint8_t *buf) {
	HBAPort *port = aport.port;
	port->is = 0xFFFFFFFF;
	uint32_t slot = find_cmdslot(aport);
	if (slot == 0xFFFFFFFF)
		return 1;

	HBACommandHeader *cmdheader = (HBACommandHeader *)aport.clb;
	cmdheader += slot;
	cmdheader->cfl = sizeof(FIS_HostToDevice) / sizeof(uint32_t);
	cmdheader->w = 0;

	HBACommandTable *cmdtable = (HBACommandTable *)aport.ctba[slot];

	cmdtable->prdt_entry[0].dba = (uint32_t)(intptr_t)get_phys_addr(buf);
	cmdtable->prdt_entry[0].dbau = 0;
	cmdtable->prdt_entry[0].dbc = (count * 512) - 1;
	cmdtable->prdt_entry[0].i = 1;

	FIS_HostToDevice *cmdfis = (FIS_HostToDevice *)(&cmdtable->cfis);

	cmdfis->FIS_Type = 0x27; // Host to device
	cmdfis->c = 1;
	cmdfis->command = SATA_READ_DMA_EX;

	cmdfis->lba0 = (uint8_t)startl;
	cmdfis->lba1 = (uint8_t)(startl >> 8);
	cmdfis->lba2 = (uint8_t)(startl >> 16);
	cmdfis->dev = 1 << 6;

	cmdfis->lba3 = (uint8_t)(startl >> 24);
	cmdfis->lba4 = (uint8_t)(starth);
	cmdfis->lba5 = (uint8_t)(starth >> 8);

	cmdfis->countl = (count & 0xFF);
	cmdfis->counth = (count >> 8);

	for (uint32_t spin = 0; spin < 1000000; spin++) {
		if (!(port->tfd & (SATA_BUSY | SATA_DRQ)))
			break;
	}
	if ((port->tfd & (SATA_BUSY | SATA_DRQ)))
		return 2;

	port->ci = (1 << slot);

	while (1) {
		if (!(port->ci & (1 << slot)))
			break;
		if (port->is & (1 << 30))
			return 3;
	}

	if (port->is & (1 << 30))
		return 3;

	return 0;
}

uint8_t ahci_write_sectors_internal(ahci_port aport, uint32_t startl, uint32_t starth, uint32_t count, uint8_t *buf) {
	HBAPort *port = aport.port;
	port->is = 0xFFFFFFFF;
	uint32_t slot = find_cmdslot(aport);
	if (slot == 0xFFFFFFFF)
		return 1;

	HBACommandHeader *cmdheader = (HBACommandHeader *)aport.clb;
	cmdheader += slot;
	cmdheader->cfl = sizeof(FIS_HostToDevice) / sizeof(uint32_t);
	cmdheader->w = 1; // We are writing this time

	HBACommandTable *cmdtable = (HBACommandTable *)aport.ctba[slot];

	cmdtable->prdt_entry[0].dba = (uint32_t)(intptr_t)get_phys_addr(buf);
	cmdtable->prdt_entry[0].dbau = 0;
	cmdtable->prdt_entry[0].dbc = (count * 512) - 1;
	cmdtable->prdt_entry[0].i = 1;

	FIS_HostToDevice *cmdfis = (FIS_HostToDevice *)(&cmdtable->cfis);

	cmdfis->FIS_Type = 0x27; // Host to device
	cmdfis->c = 1;
	cmdfis->command = SATA_WRITE_DMA_EX;

	cmdfis->lba0 = (uint8_t)startl;
	cmdfis->lba1 = (uint8_t)(startl >> 8);
	cmdfis->lba2 = (uint8_t)(startl >> 16);
	cmdfis->dev = 1 << 6;

	cmdfis->lba3 = (uint8_t)(startl >> 24);
	cmdfis->lba4 = (uint8_t)(starth);
	cmdfis->lba5 = (uint8_t)(starth >> 8);

	cmdfis->countl = (count & 0xFF);
	cmdfis->counth = (count >> 8);

	for (uint32_t spin = 0; spin < 1000000; spin++) {
		if (!(port->tfd & (SATA_BUSY | SATA_DRQ)))
			break;
	}
	if ((port->tfd & (SATA_BUSY | SATA_DRQ)))
		return 2;

	port->ci = (1 << slot);

	while (1) {
		if (!(port->ci & (1 << slot)))
			break;
		if (port->is & (1 << 30))
			return 3;
	}

	if (port->is & (1 << 30))
		return 3;

	return 0;
}

void initialize_port(ahci_port *aport) {
	HBAPort *port = aport->port;
	port->cmd &= ~HBA_CMD_ST;
	port->cmd &= ~HBA_CMD_FRE;

	while ((port->cmd & HBA_CMD_FR) || (port->cmd & HBA_CMD_CR))
		;

	void *mapped_clb = alloc_page(1);
	memset(mapped_clb, 0, 4096);
	port->clb = (uint32_t)get_phys_addr(mapped_clb);
	port->clbu = 0;
	aport->clb = mapped_clb;

	void *mapped_fb = alloc_page(1);
	memset(mapped_fb, 0, 4096);
	port->fb = (uint32_t)get_phys_addr(mapped_fb);
	port->fbu = 0;
	aport->fb = mapped_fb;

	HBACommandHeader *cmdheader = (HBACommandHeader *)mapped_clb;

	for (uint8_t i = 0; i < 32; i++) {
		cmdheader[i].prdtl = 1;
		void *ctba_buf = calloc_page(1);
		aport->ctba[i] = ctba_buf;
		cmdheader[i].ctba = (uint32_t)get_phys_addr(ctba_buf);
		cmdheader[i].ctbau = 0;
	}

	while (port->cmd & HBA_CMD_CR)
		;
	port->cmd |= HBA_CMD_FRE;
	port->cmd |= HBA_CMD_ST;
}

bool is_sata(HBAPort *port) {
	uint8_t ipm = (port->ssts >> 8) & 0xF;
	uint8_t det = (port->ssts) & 0xF;
	if (det != HBA_DET_PRESENT || ipm != HBA_IPM_ACTIVE)
		return false;
	return true;
}

void initialize_abar(HBAData *abar) {
	uint32_t pi = abar->pi;
	for (uint8_t i = 0; i < 32; i++) {
		if (pi & 1) {
			if (is_sata(&abar->ports[i])) {
				memset(&ports[num_ports], 0, 256);
				ports[num_ports].abar = abar;
				ports[num_ports].port = &abar->ports[i];
				initialize_port(&ports[num_ports]);
				sata_identify_packet info;
				ahci_identify_device(ports[num_ports], &info);
				char name[41] = {0};
				for (int i = 0; i < 40; i += 2) {
					name[i] = info.model_number[i + 1];
					name[i + 1] = info.model_number[i];
				}
				printf("[AHCI] Detected SATA drive: %s (%u MiB)\n", name, info.total_sectors / 2048);
				num_ports++;
			}
		}
		pi >>= 1;
	}
}

void print_pci_data(pci_t pci, uint8_t i, uint8_t j, uint8_t k) {
	(void)i;
	(void)j;
	(void)k;
	if (pci.vendorID != 0xFFFF && pci.class == 1 && pci.subclass == 6) {
		/*if (k==0) {
		  printf("Detected SATA Host on port %X:%X\n", i,j);
		  dprintf("Detected SATA Host on port %X:%X\n", i,j);
		} else {
		  printf("Detected SATA Host on port %X:%X.%u\n",i,j,k);
		  dprintf("Detected SATA Host on port %X:%X.%u\n",i,j,k);
		}*/
		identity_map((void *)pci.BAR5);
		initialize_abar((HBAData *)pci.BAR5);
	}
}

void init_ahci() {
	ports = alloc_page(16); // 256*256 = 65536 bytes, or 16 pages. 256 ports is probably enough

	register_pci_driver(print_pci_data, 1, 6);
	driver_id = register_disk_handler(ahci_read_sectors, ahci_write_sectors);

	kprint("[INIT] Initialized AHCI driver");
}

uint8_t ahci_read_sectors(uint16_t drive_num, uint64_t start_sector, uint32_t count, void *buf) {
	if (ports[drive_num].abar != 0)
		return ahci_read_sectors_internal(ports[drive_num], start_sector & 0xFFFFFFFF, (start_sector >> 32) & 0xFFFFFFFF, count, buf);
	else
		return 4;
}

uint8_t ahci_write_sectors(uint16_t drive_num, uint64_t start_sector, uint32_t count, void *buf) {
	if (ports[drive_num].abar != 0)
		return ahci_write_sectors_internal(ports[drive_num], start_sector & 0xFFFFFFFF, (start_sector >> 32) & 0xFFFFFFFF, count, buf);
	else
		return 4;
}

bool drive_exists(uint16_t drive_num) { return drive_num < 256 && (ports[drive_num].abar != 0); }

void print_sector(uint8_t *read) {
	for (int j = 0; j < 16; j++) {
		for (int i = 0; i < 32; i++) {
			if (read[(j * 32) + i] < 16) {
				printf("0");
			}
			printf("%X", read[(j * 32) + i]);
		}
		printf("\n");
	}
}
