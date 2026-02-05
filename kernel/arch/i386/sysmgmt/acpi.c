#include <kernel/acpi.h>

#define EDBA_PTR_ADDR (paddr_t)0x40EUL
#define EDBA_END_ADDR (paddr_t)0xA0000UL
#define ACPI_ALT_ADDR (paddr_t)0xE0000UL
#define ACPI_ALT_END_ADDR (paddr_t)0x100000UL

#define RSDP_SIGNATURE "RSD PTR "
#define RSDP_SIGNATURE_LEN 8
#define FADT_SIGNATURE "FACP"
#define FADT_SIGNATURE_LEN 4
#define SSDT_SIGNATURE "SSDT"
#define SSDT_SIGNATURE_LEN 4
#define S5_SIGNATURE "_S5_"
#define S5_SIGNATURE_LEN 4

#define ACPI_SLEEP_ENABLE (1 << 13)

#define io_align_address(addr, src_base, target_base) (void *)((uintptr_t)(addr) - (uintptr_t)src_base + (uintptr_t)target_base)

#define ACPI_ALLOC_MAX 16

typedef struct ACPI_alloc {
	void *src;
	void *target;
	size_t size;
	size_t page_count;
	unsigned short index;
	bool used;
} ACPI_alloc;

ACPI_alloc allocs[ACPI_ALLOC_MAX] = {0};
size_t alloc_count = 0;

ACPI_alloc acpi_alloc(void *ptr, size_t size) {
	size_t alloc_size = size;
	alloc_size += (uintptr_t)ptr & (PAGE_SIZE - 1);

	size_t page_count = (alloc_size + PAGE_SIZE - 1) / PAGE_SIZE;
	void *alloc = ioalloc_pages(ptr, page_count);

	if (!alloc) {
		PANIC("ACPI alloc failed: unable to allocate I/O memory");
	}

	ACPI_alloc acpi_alloc = {
		.src = ptr,
		.target = alloc,
		.size = size,
		.page_count = page_count,
		.index = 0,
		.used = true
	};

	unsigned short i = 0;
	for (; i < ACPI_ALLOC_MAX; i++) {
		if (!allocs[i].used) {
			acpi_alloc.index = i;
			allocs[i] = acpi_alloc;	
			break;
		}
	}

	if (i == ACPI_ALLOC_MAX) {
		PANIC("ACPI alloc failed: no free slots for tracking allocation");
	}

	return acpi_alloc;
}

void acpi_free(ACPI_alloc alloc) {
	allocs[alloc.index].used = false;

	for (size_t i = 0; i < alloc.page_count; i++) {
		// Free only pages which aren't also used by other allocations
		bool page_used_elsewhere = false;
		for (size_t j = 0; j < ACPI_ALLOC_MAX; j++) {
			if (allocs[j].used) {
				uintptr_t alloc_start = (uintptr_t)allocs[j].target & ~(PAGE_SIZE - 1);
				uintptr_t alloc_end = alloc_start + (allocs[j].page_count * PAGE_SIZE);
				uintptr_t page_addr = ((uintptr_t)alloc.target & ~(PAGE_SIZE - 1)) + (i * PAGE_SIZE);

				if (page_addr >= alloc_start && page_addr < alloc_end) {
					page_used_elsewhere = true;
					break;
				}
			}
		}
		
		if (!page_used_elsewhere) {
			iofree_pages(alloc.target + (i * PAGE_SIZE), 1);
		}
	}
}

typedef struct S5_bytecode {
	char signature[4];
	uint8_t pkg_op;
	uint8_t pkg_len;
	uint8_t num_elements;
	uint8_t byteprefix0;
	uint8_t sleep_type_a;
	uint8_t byteprefix1;
	uint8_t sleep_type_b;
} __attribute__((packed)) S5_bytecode;

ACPI_RSDP acpi_get_rsdp() {
	void *edba_base = ioalloc_pages(EDBA_PTR_ADDR, 1);
	uint16_t edba_ptr_val = *(volatile uint16_t *)edba_base;
	paddr_t edba_addr = (paddr_t)((uintptr_t)edba_ptr_val << 4);
	iofree_pages(edba_base, 1);

	ACPI_RSDP rsdp = {0};

	// Make sure the EDBA is in the valid range before we try to read from it
	if (edba_addr != NULL) {

		// Search through EDBA for RSDP signature
		size_t edba_area_size = 1024;
		ACPI_alloc edba_alloc = acpi_alloc((void *)edba_addr, edba_area_size);

		for (size_t i = 0; i < edba_area_size - sizeof(ACPI_RSDP); i += 16) {
			void *current_addr = (void *)((uintptr_t)edba_alloc.target + i);
			if (!memcmp(current_addr, RSDP_SIGNATURE, RSDP_SIGNATURE_LEN)) {
				memcpy(&rsdp, current_addr, sizeof(ACPI_RSDP));
				acpi_free(edba_alloc);
				return rsdp;
			}
		}

		acpi_free(edba_alloc);

	}
	

	// If we didn't find it in the EDBA, search the alternate area
	size_t alt_area_size = ACPI_ALT_END_ADDR - ACPI_ALT_ADDR;
	ACPI_alloc alt_alloc = acpi_alloc((void *)ACPI_ALT_ADDR, alt_area_size);
	for (size_t i = 0; i < alt_area_size - sizeof(ACPI_RSDP); i += 16) {
		void *current_addr = (void *)((uintptr_t)alt_alloc.target + i);
		if (!memcmp(current_addr, RSDP_SIGNATURE, RSDP_SIGNATURE_LEN)) {
			memcpy(&rsdp, current_addr, sizeof(ACPI_RSDP));
			acpi_free(alt_alloc);
			return rsdp;
		}
	}

	acpi_free(alt_alloc);

	return rsdp;
}

ACPI_FADT acpi_get_fadt() {
	ACPI_FADT fadt = {0};

	ACPI_RSDP rsdp = acpi_get_rsdp();
	
	if (memcmp(rsdp.rsdp.RSDPSig, "RSD PTR ", 8))
		return fadt; // No ACPI, can't continue

	ACPI_SDTHeader rsdt;
	void *rsdt_ptr;
	uint8_t ptr_size = 4;

	// Precalculate 32 bit or 64 bit (RSDP or XSDP)
	if (!rsdp.rsdp.rev_num) {
		rsdt_ptr = (void *)rsdp.rsdp.rsdt_addr;
		ACPI_alloc rsdt_alloc = acpi_alloc(rsdt_ptr, sizeof(ACPI_SDTHeader));
		memcpy(&rsdt, rsdt_alloc.target, sizeof(ACPI_SDTHeader));
		acpi_free(rsdt_alloc);
	} else {
		ptr_size = 8;
		rsdt_ptr = (void *)rsdp.xsdp.xsdt_low_addr;
		ACPI_alloc rsdt_alloc = acpi_alloc(rsdt_ptr, sizeof(ACPI_SDTHeader));
		memcpy(&rsdt, rsdt_alloc.target, sizeof(ACPI_SDTHeader));
		acpi_free(rsdt_alloc);
	}

	// Allocate whole RSDT/XSDT so we can read the entries
	ACPI_alloc rsdt_alloc = acpi_alloc(rsdt_ptr, rsdt.length);

	uint32_t *entries = (uint32_t *)((void *)rsdt_alloc.target + sizeof(ACPI_SDTHeader));
	uint32_t num_entries = (rsdt.length - sizeof(ACPI_SDTHeader)) / ptr_size;

	for (uint32_t i = 0; i < num_entries; i++) {
		uint32_t ptr = entries[i * (ptr_size / 4)];
		
		ACPI_alloc entry_alloc = acpi_alloc((void *)ptr, sizeof(ACPI_SDTHeader));
		ACPI_SDTHeader *is_facp = (ACPI_SDTHeader *)entry_alloc.target;

		if (!memcmp(is_facp->SDTSig, FADT_SIGNATURE, FADT_SIGNATURE_LEN)) {
			acpi_free(entry_alloc);
			acpi_free(rsdt_alloc);

			ACPI_alloc fadt_alloc = acpi_alloc((void *)ptr, sizeof(ACPI_FADT));
			fadt = *(ACPI_FADT *)fadt_alloc.target;
			acpi_free(fadt_alloc);

			return fadt;
		}

		acpi_free(entry_alloc);
	}

	acpi_free(rsdt_alloc);
	
	return fadt;
}

paddr_t acpi_find_ssdt() {
	ACPI_RSDP rsdp = acpi_get_rsdp();
	if (memcmp(rsdp.rsdp.RSDPSig, "RSD PTR ", 8))
		return NULL; // No ACPI, can't continue

	ACPI_SDTHeader rsdt;
	void *rsdt_ptr;
	uint8_t ptr_size = 4;

	// Precalculate 32 bit or 64 bit (RSDP or XSDP)
	if (!rsdp.rsdp.rev_num) {
		rsdt_ptr = (void *)rsdp.rsdp.rsdt_addr;
		ACPI_alloc rsdt_alloc = acpi_alloc(rsdt_ptr, sizeof(ACPI_SDTHeader));
		memcpy(&rsdt, rsdt_alloc.target, sizeof(ACPI_SDTHeader));
		acpi_free(rsdt_alloc);
	} else {
		ptr_size = 8;
		rsdt_ptr = (void *)rsdp.xsdp.xsdt_low_addr;
		ACPI_alloc rsdt_alloc = acpi_alloc(rsdt_ptr, sizeof(ACPI_SDTHeader));
		memcpy(&rsdt, rsdt_alloc.target, sizeof(ACPI_SDTHeader));
		acpi_free(rsdt_alloc);
	}

	// Allocate whole RSDT/XSDT so we can read the entries
	ACPI_alloc rsdt_alloc = acpi_alloc(rsdt_ptr, rsdt.length);

	uint32_t *entries = (uint32_t *)((void *)rsdt_alloc.target + sizeof(ACPI_SDTHeader));
	uint32_t num_entries = (rsdt.length - sizeof(ACPI_SDTHeader)) / ptr_size;

	for (uint32_t i = 0; i < num_entries; i++) {
		uint32_t ptr = entries[i * (ptr_size / 4)];
		
		ACPI_alloc entry_alloc = acpi_alloc((void *)ptr, sizeof(ACPI_SDTHeader));
		ACPI_SDTHeader *is_ssdt = (ACPI_SDTHeader *)entry_alloc.target;

		if (!memcmp(is_ssdt->SDTSig, SSDT_SIGNATURE, SSDT_SIGNATURE_LEN)) {
			paddr_t ssdt_addr = entry_alloc.src;
			
			acpi_free(entry_alloc);
			acpi_free(rsdt_alloc);

			return ssdt_addr;
		}

		acpi_free(entry_alloc);
	}

	acpi_free(rsdt_alloc);
	
	return NULL;
}

void acpi_list_tables() {
	ACPI_RSDP rsdp = acpi_get_rsdp();
	if (memcmp(rsdp.rsdp.RSDPSig, RSDP_SIGNATURE, RSDP_SIGNATURE_LEN))
		return; // No ACPI, can't continue

	ACPI_SDTHeader rsdt;
	void *rsdt_ptr;
	uint8_t ptr_size = 4;

	// Precalculate 32 bit or 64 bit (RSDP or XSDP)
	if (!rsdp.rsdp.rev_num) {
		rsdt_ptr = (void *)rsdp.rsdp.rsdt_addr;
		ACPI_alloc rsdt_alloc = acpi_alloc(rsdt_ptr, sizeof(ACPI_SDTHeader));
		memcpy(&rsdt, rsdt_alloc.target, sizeof(ACPI_SDTHeader));
		acpi_free(rsdt_alloc);
	} else {
		ptr_size = 8;
		rsdt_ptr = (void *)rsdp.xsdp.xsdt_low_addr;
		ACPI_alloc rsdt_alloc = acpi_alloc(rsdt_ptr, sizeof(ACPI_SDTHeader));
		memcpy(&rsdt, rsdt_alloc.target, sizeof(ACPI_SDTHeader));
		acpi_free(rsdt_alloc);
	}

	// Allocate whole RSDT/XSDT so we can read the entries
	ACPI_alloc rsdt_alloc = acpi_alloc(rsdt_ptr, rsdt.length);

	uint32_t *entries = (uint32_t *)((void *)rsdt_alloc.target + sizeof(ACPI_SDTHeader));
	uint32_t num_entries = (rsdt.length - sizeof(ACPI_SDTHeader)) / ptr_size;

	for (uint32_t i = 0; i < num_entries; i++) {
		uint32_t ptr = entries[i * (ptr_size / 4)];
		
		ACPI_alloc entry_alloc = acpi_alloc((void *)ptr, sizeof(ACPI_SDTHeader));
		ACPI_SDTHeader *is_ssdt = (ACPI_SDTHeader *)entry_alloc.target;

		kprintf("[ACPI] Table: %c%c%c%c\n", is_ssdt->SDTSig[0], is_ssdt->SDTSig[1], is_ssdt->SDTSig[2], is_ssdt->SDTSig[3]);

		acpi_free(entry_alloc);
	}

	acpi_free(rsdt_alloc);
}

void acpi_enable(ACPI_FADT fadt) {
	kprint("[ACPI] Enabling ACPI");
	if (fadt.SMI_cmdport && fadt.ACPI_enable && fadt.ACPI_disable && !(inw(fadt.PM1a_ctl_blk) & 1)) {
		outb(fadt.SMI_cmdport, fadt.ACPI_enable);
		for (uint16_t i = 0; i < 300; i++) {
			if (inw(fadt.PM1a_ctl_blk) & 1)
				break;
			sleep(10);
		}
	}
}

void acpi_disable(ACPI_FADT fadt) {
	kprint("[ACPI] Disabling ACPI");
	if (fadt.SMI_cmdport || fadt.ACPI_enable || fadt.ACPI_disable || !(inw(fadt.PM1a_ctl_blk) & 1)) {
		outb(fadt.SMI_cmdport, fadt.ACPI_disable);
		for (uint16_t i = 0; i < 300; i++) {
			if (!(inw(fadt.PM1a_ctl_blk) & 1))
				break;
			sleep(10);
		}
	}
}

// Uses kaworu's method, but not their code
// https://forum.osdev.org/viewtopic.php?f=1&t=16990
bool acpi_dt_attempt_s5(paddr_t dt, ACPI_FADT fadt) {
	ACPI_alloc dt_header = acpi_alloc(dt, sizeof(ACPI_SDTHeader));

	ACPI_SDTHeader dt_sdt_header;
	memcpy(&dt_sdt_header, dt_header.target, sizeof(ACPI_SDTHeader));
	acpi_free(dt_header);

	ACPI_alloc dt_alloc = acpi_alloc(dt, dt_sdt_header.length);

	void *dt_entries_ptr = dt_alloc.target + sizeof(dt_sdt_header);
	void *dt_entries_end = dt_alloc.target + dt_sdt_header.length - sizeof(S5_bytecode);

	for (void *searchaddr = dt_entries_ptr; searchaddr < dt_entries_end; searchaddr++) {
        if (!memcmp(searchaddr, S5_SIGNATURE, S5_SIGNATURE_LEN)) {
            uint8_t *s5_ptr = (uint8_t *)searchaddr + 4; // Skip "_S5_"
            if (*s5_ptr == 0x12) s5_ptr++; // Skip PackageOp
            
            // Rudimentary PkgLen skip (handles 1-4 byte encodings)
            if ((*s5_ptr & 0xC0) == 0) s5_ptr += 1;
            else s5_ptr += ((*s5_ptr >> 6) & 0x03) + 1;
            
            s5_ptr++; // Skip NumElements

            uint16_t SLP_TYPa, SLP_TYPb;
            if (*s5_ptr == 0x0A) s5_ptr++; // Skip BytePrefix
            SLP_TYPa = *(s5_ptr++);
            
            if (*s5_ptr == 0x0A) s5_ptr++; // Skip BytePrefix
            SLP_TYPb = *(s5_ptr++);

            outw(fadt.PM1a_ctl_blk, SLP_TYPa << 10 | ACPI_SLEEP_ENABLE);
            if (fadt.PM1b_ctl_blk)
                outw(fadt.PM1b_ctl_blk, SLP_TYPb << 10 | ACPI_SLEEP_ENABLE);

            acpi_free(dt_alloc);
            return true;
        }
    }

	acpi_free(dt_alloc);
	return false;
}

bool acpi_shutdown() {
	ACPI_FADT fadt = acpi_get_fadt();
	if (memcmp(fadt.header.SDTSig, "FACP", 4))
		return false;

	paddr_t dsdt_paddr = (paddr_t)fadt.dsdt;

	// Most likely S5 is in DSDT, so look there first.
	if (dsdt_paddr && acpi_dt_attempt_s5(dsdt_paddr, fadt)) {
		return true;
	}

	paddr_t ssdt_paddr = (paddr_t)acpi_find_ssdt();

	// Some machines put it in the SSDT for some reason, so look there next.
	if (ssdt_paddr && acpi_dt_attempt_s5(ssdt_paddr, fadt)) {
		return true;
	}

	// Sometimes ACPI populates tables on enable, so enable it and try again.
	acpi_enable(fadt);
	if (dsdt_paddr && acpi_dt_attempt_s5(dsdt_paddr, fadt)) {
		return true;
	}

	return ssdt_paddr && acpi_dt_attempt_s5(ssdt_paddr, fadt);
}

bool acpi_detect_ps2() {
	ACPI_FADT fadt = acpi_get_fadt();

	if (memcmp(fadt.header.SDTSig, "FACP", 4))
		return true; // No ACPI, assume PS/2

	return fadt.bootarchflags & ACPI_BOOTARCHFLAG_PS2;
}
