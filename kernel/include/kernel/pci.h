#ifndef _KERNEL_PCI_H
#define _KERNEL_PCI_H

#include <kernel/stdio.h>
uint16_t pci_read_config_word(uint8_t bus,uint8_t num, uint8_t function, uint8_t offset);

typedef struct {
	uint16_t vendorID;
	uint16_t deviceID;
	uint16_t command;
	uint16_t status;
	uint8_t revisionID;
	uint8_t progIF;
	uint8_t subclass;
	uint8_t classCode;
	uint8_t cacheLineSize;
	uint8_t latencyTimer;
	uint8_t headerType;
	uint8_t BIST;
	uint32_t BAR0;
	uint32_t BAR1;
	uint32_t BAR2;
	uint32_t BAR3;
	uint32_t BAR4;
	uint32_t BAR5;
	uint32_t cardbusCISPointer;
	uint16_t subsystemVendorID;
	uint16_t subsystemID;
	uint32_t expansionROMBaseAddress;
	uint8_t capabilitiesPointer;
	uint8_t reserved0;
	uint16_t reserved1;
	uint32_t reserved2;
	uint8_t interruptLine;
	uint8_t interruptPIN;
	uint8_t minGrant;
	uint8_t maxLatency;
} __attribute__((packed)) pci_t;

pci_t getPCIData(uint8_t bus, uint8_t num, uint8_t function);

#endif
