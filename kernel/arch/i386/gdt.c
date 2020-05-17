#include <kernel/ksetup.h>

typedef struct
{
   uint32_t prev_tss;
   uint32_t esp0;
   uint32_t ss0;
   uint32_t esp1;
   uint32_t ss1;
   uint32_t esp2;
   uint32_t ss2;
   uint32_t cr3;
   uint32_t eip;
   uint32_t eflags;
   uint32_t eax;
   uint32_t ecx;
   uint32_t edx;
   uint32_t ebx;
   uint32_t esp;
   uint32_t ebp;
   uint32_t esi;
   uint32_t edi;
   uint32_t es;         
   uint32_t cs;        
   uint32_t ss;        
   uint32_t ds;        
   uint32_t fs;       
   uint32_t gs;         
   uint32_t ldt;      
   uint16_t trap;
   uint16_t iomap_start;
} __attribute__((packed)) tss_entry_t;

tss_entry_t tss;

typedef struct {
	uint8_t base_l;
	//Access
	uint8_t pr:1;
	uint8_t prlvl:2;
	uint8_t s:1;
	uint8_t ex:1;
	uint8_t dc:1;
	uint8_t rw:1;
	uint8_t ac:1;
	//
	uint8_t limit:4;
	//Granularity
	uint8_t gr:1;
	uint8_t sz:1;
	uint8_t z:1;
	uint8_t a:1;
	//
	uint8_t base_h;
	
} __attribute__ ((packed)) gdt_desc_t;

#define NUM_GDT_ENTRIES 6

static uint64_t gdtEntries[NUM_GDT_ENTRIES];
typedef struct
{
  uint16_t limit;
  uint32_t firstEntryAddr;
} __attribute__ ((packed)) gdt_pointer_t;

gdt_pointer_t gdtPtr;

uint64_t create_gdt_desc(uint32_t base, uint32_t limit, uint16_t flags) {
	uint64_t gdt_entry;
 
    gdt_entry  =  limit				& 0x000F0000;
    gdt_entry |= (flags <<  8)		& 0x00F0FF00;
    gdt_entry |= (base >> 16)	& 0x000000FF;
    gdt_entry |=  base				& 0xFF000000;
 
    gdt_entry <<= 32;
 
    gdt_entry |= base  << 16;                       // set base bits 15:0
    gdt_entry |= limit  & 0x0000FFFF;               // set limit bits 15:0
 
    return gdt_entry;
}

extern void gdt_flush(uint32_t);

void init_gdt() {
	gdtPtr.limit = (sizeof(uint64_t) * NUM_GDT_ENTRIES) - 1u;
	gdtPtr.firstEntryAddr = (uint32_t)&gdtEntries;
	
	gdtEntries[0] = create_gdt_desc(0,0,0); // Null desc
	//Kernel can do whatever it wants
	gdtEntries[1] = create_gdt_desc(0,0xFFFFFFFF,0xCF9A); //Kernel code
	gdtEntries[2] = create_gdt_desc(0,0xFFFFFFFF,0xCF92); // Kernel data
	//In the future, change mapping to where programs will be loaded.
	gdtEntries[3] = create_gdt_desc(0,0xFFFFFFFF,0xCFFA); //User code
	gdtEntries[4] = create_gdt_desc(0,0xFFFFFFFF,0xCFF2); //User data
	gdtEntries[5] = create_gdt_desc((uint32_t)&tss,(uint32_t)&tss+sizeof(tss)+1,0x89);
	
	gdt_flush((uint32_t)&gdtPtr);
	kprint("[INIT] GDT Enabled");
}
