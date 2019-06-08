#include <kernel/idt.h>

//(pretty heavily) Based on https://wiki.osdev.org/Interrupts_tutorial

struct IDT_entry_t {
	uint16_t bits_offset_lower;
	uint16_t sel;
	uint8_t zero;
	uint8_t type_attr;
	uint16_t bits_offset_higher;
};

struct IDT_entry_t IDT[256];

extern int load_idt();
extern int irq0();
extern int irq1();
extern int irq2();
extern int irq3();
extern int irq4();
extern int irq5();
extern int irq6();
extern int irq7();
extern int irq8();
extern int irq9();
extern int irq10();
extern int irq11();
extern int irq12();
extern int irq13();
extern int irq14();
extern int irq15();

//For simplicity, as it is very repetitive in the Interrupts_tutorial
void set_idt_values(uint8_t _idt, uint32_t irq_addr) {
	IDT[_idt].bits_offset_lower = irq_addr & 0xFFFF;
	IDT[_idt].sel = 0x08;
	IDT[_idt].zero = 0;
	IDT[_idt].type_attr = 0x8E;
	IDT[_idt].bits_offset_higher = (irq_addr & 0xFFFF0000) >> 16;
}

void init_idt() {
	uint32_t irq0_addr;
    uint32_t irq1_addr;
    uint32_t irq2_addr;
    uint32_t irq3_addr;
    uint32_t irq4_addr;
    uint32_t irq5_addr;
    uint32_t irq6_addr;
    uint32_t irq7_addr;
    uint32_t irq8_addr;
    uint32_t irq9_addr;
    uint32_t irq10_addr;
    uint32_t irq11_addr;
    uint32_t irq12_addr;
    uint32_t irq13_addr;
    uint32_t irq14_addr;
    uint32_t irq15_addr;
	
	uint32_t idt_addr;
	uint32_t idt_ptr[2];
	
	//Remap the PIC
	outb(0x20, 0x11);
	outb(0xA0, 0x11);
	outb(0x21, 0x20);
	outb(0xA1, 40);
	outb(0x21, 0x04);
	outb(0xA1, 0x02);
	outb(0x21, 0x01);
	outb(0xA1, 0x01);
	outb(0x21, 0x0);
	outb(0xA1, 0x0);
	
	irq0_addr = (uint32_t) irq0;
	set_idt_values(32,irq0_addr);
	
	irq1_addr = (uint32_t) irq1;
	set_idt_values(33,irq1_addr);
	
	irq2_addr = (uint32_t) irq2;
	set_idt_values(34,irq2_addr);
	
	irq3_addr = (uint32_t) irq3;
	set_idt_values(35,irq3_addr);
	
	irq4_addr = (uint32_t) irq4;
	set_idt_values(36,irq4_addr);
	
	irq5_addr = (uint32_t) irq5;
	set_idt_values(37,irq5_addr);
	
	irq6_addr = (uint32_t) irq6;
	set_idt_values(38,irq6_addr);
	
	irq7_addr = (uint32_t) irq7;
	set_idt_values(39,irq7_addr);
	
	irq8_addr = (uint32_t) irq8;
	set_idt_values(40,irq8_addr);
	
	irq9_addr = (uint32_t) irq9;
	set_idt_values(41,irq9_addr);
	
	irq10_addr = (uint32_t) irq10;
	set_idt_values(42,irq10_addr);
	
	irq11_addr = (uint32_t) irq11;
	set_idt_values(43,irq11_addr);
	
	irq12_addr = (uint32_t) irq12;
	set_idt_values(44,irq12_addr);
	
	irq13_addr = (uint32_t) irq13;
	set_idt_values(45,irq13_addr);
	
	irq14_addr = (uint32_t) irq14;
	set_idt_values(46,irq14_addr);
	
	irq15_addr = (uint32_t) irq15;
	set_idt_values(47,irq15_addr);
	
	idt_addr = (uint32_t)IDT;
	idt_ptr[0] = (sizeof (struct IDT_entry_t) * 256) + ((idt_addr & 0xffff) << 16);
	idt_ptr[1] = idt_addr >> 16;
	
	load_idt(idt_ptr);
}

_Bool irq_finished[16];

_Bool has_irq_finished(uint8_t irq) {
	if (irq_finished[irq]) {
		irq_finished[irq] = false;
		return true;
	}
	return false;
}

void clear_irq_status(uint8_t irq) {
	irq_finished[irq] = false;
}

void set_irq_finish_state(uint8_t irq, _Bool state) {
	irq_finished[irq] = state;
}

void irq0_handler(void) {
	outb(0x20, 0x20); //EOI
	irq_finished[0] = true;
}
 
void irq1_handler(void) {
	kbd_handler();
	outb(0x20, 0x20); //EOI
	irq_finished[1] = true;
}
 
void irq2_handler(void) {
	outb(0x20, 0x20); //EOI
	irq_finished[2] = true;
}
 
void irq3_handler(void) {
	outb(0x20, 0x20); //EOI
	irq_finished[3] = true;
}
 
void irq4_handler(void) {
	outb(0x20, 0x20); //EOI
	irq_finished[4] = true;
}
 
void irq5_handler(void) {
	outb(0x20, 0x20); //EOI
	irq_finished[5] = true;
}
 
void irq6_handler(void) {
	outb(0x20, 0x20); //EOI
	irq_finished[6] = true;
}
 
void irq7_handler(void) {
	outb(0x20, 0x20); //EOI
	irq_finished[7] = true;
}
 
void irq8_handler(void) {
	outb(0xA0, 0x20);
	outb(0x20, 0x20); //EOI          
	irq_finished[8] = true;
}
 
void irq9_handler(void) {
	outb(0xA0, 0x20);
	outb(0x20, 0x20); //EOI
	irq_finished[9] = true;
}
 
void irq10_handler(void) {
	outb(0xA0, 0x20);
	outb(0x20, 0x20); //EOI
	irq_finished[10] = true;
}
 
void irq11_handler(void) {
	outb(0xA0, 0x20);
	outb(0x20, 0x20); //EOI
	irq_finished[11] = true;
}
 
void irq12_handler(void) {
	outb(0xA0, 0x20);
	outb(0x20, 0x20); //EOI
	irq_finished[12] = true;
}
 
void irq13_handler(void) {
	outb(0xA0, 0x20);
	outb(0x20, 0x20); //EOI
	irq_finished[13] = true;
}
 
void irq14_handler(void) {
	outb(0xA0, 0x20);
	outb(0x20, 0x20); //EOI
	irq_finished[14] = true;
}
 
void irq15_handler(void) {
	outb(0xA0, 0x20);
	outb(0x20, 0x20); //EOI
	irq_finished[15] = true;
}