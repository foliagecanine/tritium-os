#include <kernel/acpi.h>

uint32_t acpi_find_rsdp() {
	identity_map(0); //BDA is at 0x400
	uint32_t ebdastart = *(uint16_t *)0x413; //Length of memory before EBDA is at 0x413
	unmap_vaddr(0); //Unmap it so we dont get random stuff when null pointers occur.
	ebdastart<<=10; //Value is in KiB. Convert to bytes.
	uint32_t ebdalen = 0xA0000-ebdastart;
	//printf("EBDA: %#, length: %$\n",(uint64_t)ebdastart,(uint32_t)ebdalen);
	uint32_t ebdaend = ebdastart+ebdalen;
	for (uint32_t i = ebdastart; i<ebdaend; i+=4096) {
		identity_map((void *)i);
	}
	//RSD PTR is always 16 byte aligned
	for (uint32_t i = ebdastart; i < ebdaend; i+=16) {
		if (!memcmp((void *)i,"RSD PTR ", 8)) {
			for (uint32_t i = ebdastart; i<ebdaend; i+=4096) {
				unmap_vaddr((void *)i);
			}
			return i;
		}
	}
	for (uint32_t i = ebdastart; i<ebdaend; i+=4096) {
		unmap_vaddr((void *)i);
	}
	for (uint32_t i = 0xE0000; i<0xFFFFF; i+=4096) {
		identity_map((void *)i);
	}
	//Not in the EBDA? Let's try 0xE0000 to 0xFFFFF
	for (uint32_t i = 0xE0000; i<0xFFFFF; i+=16) {
		if (!memcmp((void *)i,"RSD PTR ", 8)) {
			for (uint32_t i = 0xE0000; i<0xFFFFF; i+=4096) {
				unmap_vaddr((void *)i);
			}
			return i;
		}
	}
	for (uint32_t i = 0xE0000; i<0xFFFFF; i+=4096) {
		unmap_vaddr((void *)i);
	}
	//No ACPI :(
	return 0;
}

uint32_t acpi_find_fadt() {
	ACPI_RSDPDescriptor *rsdp = (ACPI_RSDPDescriptor *)acpi_find_rsdp();
	if (!rsdp)
		return 0; //No ACPI, can't continue
	identity_map(rsdp); //Map it so we can use it
	ACPI_SDTHeader *rsdt;
	uint8_t ptr_size = 4;
	
	//Precalculate 32 bit or 64 bit (RSDP or XSDP)
	if (!rsdp->rev_num) {
		printf("Version 1.0\n");
		printf("Address: %p\n",rsdp->rsdt_addr);
		rsdt = (ACPI_SDTHeader *)rsdp->rsdt_addr;
		identity_map(rsdt);
	} else {
		printf("Version 2.0\n");
		ptr_size = 8;
		ACPI_RSDPDescriptorV2 *xsdp = (ACPI_RSDPDescriptorV2 *)rsdp;
		printf("Address: %p\n",xsdp->xsdt_low_addr);
		rsdt = (ACPI_SDTHeader *)xsdp->xsdt_low_addr;
		identity_map(rsdt);
	}
	
	uint32_t *entries = (uint32_t *)((void *)rsdt+sizeof(ACPI_SDTHeader));
	uint32_t num_entries = (rsdt->length - sizeof(ACPI_SDTHeader))/ptr_size;
	
	for (uint32_t i = 0; i < num_entries; i++) {
		uint32_t ptr = entries[i*(ptr_size/4)];
		identity_map((void *)ptr);
		ACPI_SDTHeader *is_facp = (ACPI_SDTHeader *)ptr;
		if (!memcmp(is_facp,"FACP",4))
			return (uint32_t)is_facp;
	}
	
	return 0;
}

uint32_t acpi_find_ssdt() {
	ACPI_RSDPDescriptor *rsdp = (ACPI_RSDPDescriptor *)acpi_find_rsdp();
	if (!rsdp)
		return 0; //No ACPI, can't continue
	identity_map(rsdp); //Map it so we can use it
	ACPI_SDTHeader *rsdt;
	uint8_t ptr_size = 4;
	
	//Precalculate 32 bit or 64 bit (RSDP or XSDP)
	if (!rsdp->rev_num) {
		printf("Version 1.0\n");
		printf("Address: %p\n",rsdp->rsdt_addr);
		rsdt = (ACPI_SDTHeader *)rsdp->rsdt_addr;
		identity_map(rsdt);
	} else {
		printf("Version 2.0\n");
		ptr_size = 8;
		ACPI_RSDPDescriptorV2 *xsdp = (ACPI_RSDPDescriptorV2 *)rsdp;
		printf("Address: %p\n",xsdp->xsdt_low_addr);
		rsdt = (ACPI_SDTHeader *)xsdp->xsdt_low_addr;
		identity_map(rsdt);
	}
	
	uint32_t *entries = (uint32_t *)((void *)rsdt+sizeof(ACPI_SDTHeader));
	uint32_t num_entries = (rsdt->length - sizeof(ACPI_SDTHeader))/ptr_size;
	
	for (uint32_t i = 0; i < num_entries; i++) {
		uint32_t ptr = entries[i*(ptr_size/4)];
		identity_map((void *)ptr);
		ACPI_SDTHeader *is_ssdt = (ACPI_SDTHeader *)ptr;
		if (!memcmp(is_ssdt,"SSDT",4)) {
			return (uint32_t)is_ssdt;
		}
	}
	
	return 0;
}

void acpi_list_tables() {
	ACPI_RSDPDescriptor *rsdp = (ACPI_RSDPDescriptor *)acpi_find_rsdp();
	if (!rsdp)
		return; //No ACPI, can't continue
	identity_map(rsdp); //Map it so we can use it
	ACPI_SDTHeader *rsdt;
	uint8_t ptr_size = 4;
	
	//Precalculate 32 bit or 64 bit (RSDP or XSDP)
	if (!rsdp->rev_num) {
		printf("Version 1.0\n");
		printf("Address: %p\n",rsdp->rsdt_addr);
		rsdt = (ACPI_SDTHeader *)rsdp->rsdt_addr;
		identity_map(rsdt);
	} else {
		printf("Version 2.0\n");
		ptr_size = 8;
		ACPI_RSDPDescriptorV2 *xsdp = (ACPI_RSDPDescriptorV2 *)rsdp;
		printf("Address: %p\n",xsdp->xsdt_low_addr);
		rsdt = (ACPI_SDTHeader *)xsdp->xsdt_low_addr;
		identity_map(rsdt);
	}
	
	uint32_t *entries = (uint32_t *)((void *)rsdt+sizeof(ACPI_SDTHeader));
	uint32_t num_entries = (rsdt->length - sizeof(ACPI_SDTHeader))/ptr_size;
	
	for (uint32_t i = 0; i < num_entries; i++) {
		uint32_t ptr = entries[i*(ptr_size/4)];
		identity_map((void *)ptr);
		ACPI_SDTHeader *is_facp = (ACPI_SDTHeader *)ptr;
		printf("Table: %c%c%c%c\n",((char *)is_facp)[0],((char *)is_facp)[1],((char *)is_facp)[2],((char *)is_facp)[3]);
		sleep(1000);
	}
}

void acpi_enable(ACPI_FADT *fadt) {
	kprint("[ACPI] Enabling ACPI");
	if (fadt->SMI_cmdport&&fadt->ACPI_enable&&fadt->ACPI_disable&&!(inw(fadt->PM1a_ctl_blk)&1)) {
		printf("Enabling ACPI...\n");
		printf("outb(0x%X,0x%X)\n",fadt->SMI_cmdport,fadt->ACPI_enable);
		outb(fadt->SMI_cmdport,fadt->ACPI_enable);
		for (uint16_t i = 0; i < 300; i++) {
			if (inw(fadt->PM1a_ctl_blk)&1)
				break;
			sleep(10);
		}
	}
}

void acpi_disable(ACPI_FADT *fadt) {
	kprint("[ACPI] Disabling ACPI");
	if (fadt->SMI_cmdport||fadt->ACPI_enable||fadt->ACPI_disable||!(inw(fadt->PM1a_ctl_blk)&1)) {
		printf("Disabling ACPI...\n");
		printf("outb(0x%X,0x%X)\n",fadt->SMI_cmdport,fadt->ACPI_enable);
		outb(fadt->SMI_cmdport,fadt->ACPI_disable);
		for (uint16_t i = 0; i < 300; i++) {
			if (!inw(fadt->PM1a_ctl_blk)&1)
				break;
			sleep(10);
		}
	}
}

//Uses kaworu's method, but not their code
//https://forum.osdev.org/viewtopic.php?f=1&t=16990
bool acpi_dt_attempt_s5(ACPI_SDTHeader *dt, ACPI_FADT *fadt) {
	for (void *searchaddr = dt+sizeof(dt); searchaddr < (void *)(dt+dt->length); searchaddr++) {
		identity_map(searchaddr);
		if (!memcmp(searchaddr,"_S5_",4)) {
			uint8_t *s5_bytes = searchaddr;
			//printf("S5: %#\n",(uint64_t)s5_bytes);
			if (s5_bytes[7]==0x0A)
				s5_bytes++;
			outw(fadt->PM1a_ctl_blk,s5_bytes[7]<<10|1<<13);
			outw(fadt->PM1b_ctl_blk,s5_bytes[9]<<10|1<<13);
			return true;
		}
	}
	return false;
}

bool acpi_shutdown() {
	ACPI_FADT *fadt = (ACPI_FADT *)acpi_find_fadt();
	if (!fadt)
		return false;
	identity_map(fadt);
	ACPI_SDTHeader *dsdt = (ACPI_SDTHeader *)fadt->dsdt;
	identity_map(dsdt);
	for (uint32_t i = (uint32_t)dsdt; i < (uint32_t)dsdt+dsdt->length; i+=4096) {
		identity_map((void *)i);
	}
	ACPI_SDTHeader *ssdt = (ACPI_SDTHeader *)acpi_find_ssdt();
	
	if (!ssdt) {
		if(!acpi_dt_attempt_s5(dsdt,fadt)) {
			acpi_enable(fadt);
			return acpi_dt_attempt_s5(dsdt,fadt);
		}
		return true;
	}
	
	identity_map(ssdt);
	for (uint32_t i = (uint32_t)ssdt; i < (uint32_t)ssdt+ssdt->length; i+=4096) {
		identity_map((void *)i);
	}

	if (ssdt->length>dsdt->length) {
		if (acpi_dt_attempt_s5(dsdt,fadt))
			return true;
		if (!acpi_dt_attempt_s5(ssdt,fadt)) {
			acpi_enable(fadt);
			if (acpi_dt_attempt_s5(dsdt,fadt))
				return true;
			return acpi_dt_attempt_s5(ssdt,fadt);
		}
	} else {
		if (acpi_dt_attempt_s5(ssdt,fadt))
			return true;
		if (!acpi_dt_attempt_s5(dsdt,fadt)) {
			acpi_enable(fadt);
			if (acpi_dt_attempt_s5(ssdt,fadt))
				return true;
			return acpi_dt_attempt_s5(dsdt,fadt);
		}
	}
	return false;
}