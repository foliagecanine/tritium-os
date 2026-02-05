// Uses structs based on
// https://wiki.osdev.org/RSDP
// https://wiki.osdev.org/RSDT
// https://wiki.osdev.org/XSDT
// https://wiki.osdev.org/FADT
//
// Used under CC0 license 
// (see https://forum.osdev.org/viewtopic.php?p=196531&f=1#p196531
//	and https://wiki.osdev.org/OSDev_Wiki:Copyrights)

#ifndef _KERNEL_ACPI_H
#define _KERNEL_ACPI_H

#include <kernel/stdio.h>
#include <kernel/mem.h>

typedef struct {
	char SDTSig[4];
	uint32_t length;
	uint8_t rev_num;
	uint8_t checksum;
	char OEMID[6];
	char OEMTableID[8];
	uint32_t OEM_rev;
	uint32_t creatorID;
	uint32_t creator_rev;
} __attribute__((packed)) ACPI_SDTHeader;

typedef struct {
	char RSDPSig[8];
	uint8_t checksum;
	char OEMID[6];
	uint8_t rev_num;
	uint32_t rsdt_addr;
} __attribute__((packed)) ACPI_RSDPDescriptor;

typedef struct {
	ACPI_RSDPDescriptor RSDPv1;
	uint32_t length;
	uint32_t xsdt_low_addr;
	uint32_t xsdt_high_addr;
	uint8_t checksum_ext;
	uint8_t reserved[3];
} __attribute__((packed)) ACPI_RSDPDescriptorV2;

typedef union ACPI_RSDP {
	ACPI_RSDPDescriptor rsdp;
	ACPI_RSDPDescriptorV2 xsdp;
} ACPI_RSDP;

typedef struct {
	ACPI_SDTHeader header;
	uint32_t firmware_ctl_ptr;
	uint32_t dsdt;
	uint8_t reserved0;
	uint8_t pref_PM_profile;
	uint16_t SCI_int;
	uint32_t SMI_cmdport;
	uint8_t ACPI_enable;
	uint8_t ACPI_disable;
	uint8_t S4BIOS_req;
	uint8_t PSTATE_ctl;
	uint32_t PM1a_evt_blk;
	uint32_t PM1b_evt_blk;
	uint32_t PM1a_ctl_blk;
	uint32_t PM1b_ctl_blk;
	uint32_t PM2_ctl_blk;
	uint32_t PMTimer_blk;
	uint32_t GPE0_blk;
	uint32_t GPE1_blk;
	uint8_t PM1_evt_len;
	uint8_t PM2_evt_len;
	uint8_t PMTimer_len;
	uint8_t GPE0_len;
	uint8_t GPE1_len;
	uint8_t GPE1_base;
	uint8_t CState_ctl;
	uint16_t worst_c2_latency;
	uint16_t worst_c3_latency;
	uint16_t flush_size;
	uint16_t flush_stride;
	uint8_t duty_offset;
	uint8_t duty_width;
	uint8_t day_alarm;
	uint8_t month_alarm;
	uint8_t century;
	uint16_t bootarchflags;
	uint8_t reserved1;
	uint32_t flags;
	uint8_t resetreg[12];
	uint8_t reset_value;
	uint8_t reserved2[3];
	uint64_t x_firmware_ctl;
	uint64_t x_dsdt;
	uint8_t x_PM1a_evt_blk[12];
	uint8_t x_PM1b_evt_blk[12];
	uint8_t x_PM1a_ctl_blk[12];
	uint8_t x_PM1b_ctl_blk[12];
	uint8_t x_PM2_ctl_blk[12];
	uint8_t x_PMTimer_blk[12];
	uint8_t x_GPE0_blk[12];
	uint8_t x_GPE1_blk[12];
} __attribute__((packed)) ACPI_FADT;

#define ACPI_BOOTARCHFLAG_PS2 (1 << 1)

bool acpi_detect_ps2();
ACPI_RSDP acpi_find_rsdp();
ACPI_FADT acpi_find_fadt();
void acpi_list_tables();
bool acpi_shutdown();

#endif