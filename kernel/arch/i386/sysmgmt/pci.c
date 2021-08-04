#include <kernel/pci.h>

uint32_t pci_read_config_dword(uint8_t bus,uint8_t num, uint8_t function, uint8_t offset) {
	uint32_t address = (1<<31) | (bus<<16) | (num<<11) | (function<<8) | (offset);
	outl(0xCF8,address);
	return inl(0xCFC);
}

uint16_t pci_read_config_word(uint8_t bus,uint8_t num, uint8_t function, uint8_t offset) {
	uint32_t address = (1<<31) | (bus<<16) | (num<<11) | (function<<8) | (offset & 0xfc);
	outl(0xCF8,address);
	return (uint16_t)((inl(0xCFC) >> ((offset & 2) * 8)) & 0xffff);
}

void pci_write_config_dword(uint8_t bus,uint8_t num, uint8_t function, uint8_t offset, uint32_t value) {
	uint32_t address = (1<<31) | (bus<<16) | (num<<11) | (function<<8) | (offset);
	outl(0xCF8,address);
	outl(0xCFC,value);
}

void pci_write_config_byte(uint8_t bus,uint8_t num, uint8_t function, uint8_t offset, uint8_t value) {
	uint32_t address = (1<<31) | (bus<<16) | (num<<11) | (function<<8) | (offset & 0xfc);
	outl(0xCF8,address);
	outl(0xCFC,((inl(0xCFC)&~(0xFF<<(offset&3))) | (value << ((offset&3)*8)))); 
}

pci_t getPCIData(uint8_t bus, uint8_t num, uint8_t function) {
	pci_t pciData;
	uint16_t *p = (uint16_t *)&pciData;
	for (uint8_t i = 0; i < 32; i++) {
		p[i]=pci_read_config_word(bus,num,function,i*2);
	}
	return pciData;
}

// Offset 0x3C
// 0x12345678 & ~(0xFF<<(0x3C&3)) = 0x12345678 & ~(0xFF<<0) = 0x12345678 & 0xFFFFFF00 = 0x12345600
// 0x12345600 | (0x0A << ((0x3C&3)*8)) = 0x12345600 | (0x0A << (0*8)) = 0x1234560A