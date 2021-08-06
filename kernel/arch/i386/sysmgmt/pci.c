#include <kernel/pci.h>

typedef struct {
	bool used;
	uint8_t class;
	uint8_t subclass;
	void (*pci_function)(pci_t, uint8_t, uint8_t, uint8_t);
} pci_driver;

// Save space for 255 drivers to register.
pci_driver pci_drivers[255];

bool register_pci_driver(void (*pci_function)(pci_t, uint8_t, uint8_t, uint8_t), uint8_t class, uint8_t subclass) {
	for (uint16_t i = 0; i < 255; i++) {
		if (!pci_drivers[i].used) {
			pci_drivers[i].used = true;
			pci_drivers[i].class = class;
			pci_drivers[i].subclass = subclass;
			pci_drivers[i].pci_function = pci_function;
			return true;
		}
	}
	return false;
}

void launch_driver(pci_t pci, uint8_t bus, uint8_t device, uint8_t function) {
	for (uint16_t i = 0; i < 255; i++) {
		if (pci_drivers[i].used && pci_drivers[i].class == pci.class && pci_drivers[i].subclass == pci.subclass) {
			pci_drivers[i].pci_function(pci, bus, device, function);
		}
	}
}

void pci_scan() {
	kprint("[INIT] Scanning PCI bus");
	pci_t c_pci;
	for (uint16_t i = 0; i < 256; i++) {
		c_pci = get_pci_data(i, 0, 0);
		if (c_pci.vendorID != 0xFFFF) {
			for (uint8_t j = 0; j < 32; j++) {
				c_pci = get_pci_data(i, j, 0);
				if (c_pci.vendorID != 0xFFFF) {
					launch_driver(c_pci, i, j, 0);
					for (uint8_t k = 1; k < 8; k++) {
						pci_t pci = get_pci_data(i, j, k);
						if (pci.vendorID != 0xFFFF) {
							launch_driver(pci, i, j, k);
						}
					}
				}
			}
		}
	}
	kprint("[INIT] Finished scanning PCI bus");
}

uint32_t pci_read_config_dword(uint8_t bus, uint8_t num, uint8_t function, uint8_t offset) {
	uint32_t address = (1 << 31) | (bus << 16) | (num << 11) | (function << 8) | (offset);
	outl(0xCF8, address);
	return inl(0xCFC);
}

uint16_t pci_read_config_word(uint8_t bus, uint8_t num, uint8_t function, uint8_t offset) {
	uint32_t address = (1 << 31) | (bus << 16) | (num << 11) | (function << 8) | (offset & 0xfc);
	outl(0xCF8, address);
	return (uint16_t)((inl(0xCFC) >> ((offset & 2) * 8)) & 0xffff);
}

void pci_write_config_dword(uint8_t bus, uint8_t num, uint8_t function, uint8_t offset, uint32_t value) {
	uint32_t address = (1 << 31) | (bus << 16) | (num << 11) | (function << 8) | (offset);
	outl(0xCF8, address);
	outl(0xCFC, value);
}

void pci_write_config_byte(uint8_t bus, uint8_t num, uint8_t function, uint8_t offset, uint8_t value) {
	uint32_t address = (1 << 31) | (bus << 16) | (num << 11) | (function << 8) | (offset & 0xfc);
	outl(0xCF8, address);
	outl(0xCFC, ((inl(0xCFC) & ~(0xFF << (offset & 3))) | (value << ((offset & 3) * 8))));
}

pci_t get_pci_data(uint8_t bus, uint8_t num, uint8_t function) {
	pci_t     pciData;
	uint16_t *p = (uint16_t *)&pciData;
	for (uint8_t i = 0; i < 32; i++) {
		p[i] = pci_read_config_word(bus, num, function, i * 2);
	}
	return pciData;
}

// Offset 0x3C
// 0x12345678 & ~(0xFF<<(0x3C&3)) = 0x12345678 & ~(0xFF<<0) = 0x12345678 & 0xFFFFFF00 = 0x12345600
// 0x12345600 | (0x0A << ((0x3C&3)*8)) = 0x12345600 | (0x0A << (0*8)) = 0x1234560A
