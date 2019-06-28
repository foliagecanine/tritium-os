#include <kernel/init.h> 
#include <kernel/kprint.h>

//A lot of this code is based on https://wiki.osdev.org/Getting_to_Ring_3

#define SEG_DESCTYPE(x)  ((x) << 0x04) // Descriptor type (0 for system, 1 for code/data)
#define SEG_PRES(x)      ((x) << 0x07) // Present
#define SEG_SAVL(x)      ((x) << 0x0C) // Available for system use
#define SEG_LONG(x)      ((x) << 0x0D) // Long mode
#define SEG_SIZE(x)      ((x) << 0x0E) // Size (0 for 16-bit, 1 for 32)
#define SEG_GRAN(x)      ((x) << 0x0F) // Granularity (0 for 1B - 1MB, 1 for 4KB - 4GB)
#define SEG_PRIV(x)     (((x) &  0x03) << 0x05)   // Set privilege level (0 - 3)
 
#define SEG_DATA_RD        0x00 // Read-Only
#define SEG_DATA_RDA       0x01 // Read-Only, accessed
#define SEG_DATA_RDWR      0x02 // Read/Write
#define SEG_DATA_RDWRA     0x03 // Read/Write, accessed
#define SEG_DATA_RDEXPD    0x04 // Read-Only, expand-down
#define SEG_DATA_RDEXPDA   0x05 // Read-Only, expand-down, accessed
#define SEG_DATA_RDWREXPD  0x06 // Read/Write, expand-down
#define SEG_DATA_RDWREXPDA 0x07 // Read/Write, expand-down, accessed
#define SEG_CODE_EX        0x08 // Execute-Only
#define SEG_CODE_EXA       0x09 // Execute-Only, accessed
#define SEG_CODE_EXRD      0x0A // Execute/Read
#define SEG_CODE_EXRDA     0x0B // Execute/Read, accessed
#define SEG_CODE_EXC       0x0C // Execute-Only, conforming
#define SEG_CODE_EXCA      0x0D // Execute-Only, conforming, accessed
#define SEG_CODE_EXRDC     0x0E // Execute/Read, conforming
#define SEG_CODE_EXRDCA    0x0F // Execute/Read, conforming, accessed
 
#define GDT_CODE_PL0 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
                     SEG_LONG(0)     | SEG_SIZE(1) | SEG_GRAN(1) | \
                     SEG_PRIV(0)     | SEG_CODE_EXRD
 
#define GDT_DATA_PL0 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
                     SEG_LONG(0)     | SEG_SIZE(1) | SEG_GRAN(1) | \
                     SEG_PRIV(0)     | SEG_DATA_RDWR
 
#define GDT_CODE_PL3 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
                     SEG_LONG(0)     | SEG_SIZE(1) | SEG_GRAN(1) | \
                     SEG_PRIV(3)     | SEG_CODE_EXRD
 
#define GDT_DATA_PL3 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
                     SEG_LONG(0)     | SEG_SIZE(1) | SEG_GRAN(1) | \
                     SEG_PRIV(3)     | SEG_DATA_RDWR

//We need this struct in order to set up tss in the GDT.
//We won't end up needing this until we actually want to enter Ring 3.
struct tss_entry
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
} __packed;
 
typedef struct tss_entry tss_entry_t;

tss_entry_t tss; //actually put the tss into memory

#define NUM_GDT_ENTRIES 6

static uint64_t gdtEntries[NUM_GDT_ENTRIES];
static struct gdt_pointer
{
  uint16_t limit;
  uint32_t firstEntryAddr;
} __attribute__ ((packed)) gdtPtr;

uint64_t gdt_encode(uint32_t base, uint32_t limit, uint16_t flag)
{
    uint64_t gdt_entry;
 
    gdt_entry  =  limit       & 0x000F0000;
    gdt_entry |= (flag <<  8) & 0x00F0FF00;
    gdt_entry |= (base >> 16) & 0x000000FF;
    gdt_entry |=  base        & 0xFF000000;
 
    gdt_entry <<= 32;
 
    gdt_entry |= base  << 16;                       // set base bits 15:0
    gdt_entry |= limit  & 0x0000FFFF;               // set limit bits 15:0
 
    return gdt_entry;
}

extern void gdt_flush(uint32_t);
//extern void enter_usermode_fully();
extern void tss_flush();

//Set up GDT. We will need to set up the tss later, but oh well
void initialize_gdt(uint32_t total_mem) {
	gdtPtr.limit = (sizeof(uint64_t) * NUM_GDT_ENTRIES) - 1u;
	gdtPtr.firstEntryAddr = (uint32_t)&gdtEntries;

	gdtEntries[0] = gdt_encode(0, 0, 0);
	
	//Had to redo GDT so usermode could execute code that the kernel loaded. See old code at kernel/arch/i386/tests.c
	
	gdtEntries[1] = gdt_encode(0,0xFFFFFFFF,0xCF9A);
	gdtEntries[2] = gdt_encode(0,0xFFFFFFFF,0xCF92);
	gdtEntries[3] = gdt_encode(0,0xFFFFFFFF,0xCFFA);
	gdtEntries[4] = gdt_encode(0,0xFFFFFFFF,0xCFF2);

	gdt_flush((uint32_t)&gdtPtr);
	kprint("Set up GDT");
}

void install_tss () {
	uint32_t base = (uint32_t) &tss;
	
	gdtEntries[5] = gdt_encode(base,base+sizeof(tss_entry_t),0xE9);
	
	memset((void*) &tss, 0, sizeof(tss_entry_t));
	
	tss.ss0 = 0x10;
	uint32_t stack_ptr = 0;
	asm("mov %%esp, %0" : "=r"(stack_ptr));
	tss.esp0 = stack_ptr;
	
	tss.cs = 0x0B;
	tss.ss = 0x13;
	tss.ds = 0x13;
	tss.es = 0x13;
	tss.fs = 0x13;
	tss.gs =0x13;
	
	gdt_flush((uint32_t)&gdtPtr);
	tss_flush();
}

int a(int b) {
	return b*b+b;
}

void enter_usermode () { //There's no going back...
	install_tss();
	
	asm volatile("\
		cli; \
		mov $0x23, %ax; \
		mov %ax, %ds; \
		mov %ax, %es; \
		mov %ax, %fs; \
		mov %ax, %gs; \
		push $0x23; \
		push %esp; \
		pushf; \
		push $0x1B; \
		push $1f; \
		iret; \
		1: \
		pop %eax\
	");
	//Line 195 (cant do inline assembly comments): Fix the stack. It has an extra item.
}

void tss_stack_set (uint16_t ss, uint16_t esp) {
	tss.ss0 = ss;
	tss.esp0 = esp;
}
