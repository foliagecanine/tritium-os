#include <kernel/pci.h>

uint16_t pci_read_config_word(uint8_t bus,uint8_t num, uint8_t function, uint8_t offset) {
	uint32_t address = (1<<31) | (bus<<16) | (num<<11) | (function<<8) | (offset & 0xfc);
	outl(0xCF8,address);
	return (uint16_t)((inl(0xCFC) >> ((offset & 2) * 8)) & 0xffff);
}

pci_t getPCIData(uint8_t bus, uint8_t num, uint8_t function) {
	pci_t pciData;
	uint16_t *p = (uint16_t *)&pciData;
	for (uint8_t i = 0; i < 32; i++) {
		p[i]=pci_read_config_word(bus,num,function,i*2);
	}
	return pciData;
}
