#include <kernel/init.h> 
#include <kernel/kprint.h>

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
void initialize_gdt() {
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
