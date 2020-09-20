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
extern int default_handler();
extern uint64_t ticks;

//For simplicity, as it is very repetitive in the Interrupts_tutorial
void idt_install_interrupt_request(uint8_t _idt, uint32_t irq_addr, uint16_t type_flags) {
	IDT[_idt].bits_offset_lower = irq_addr & 0xFFFF;
	IDT[_idt].sel = 0x08;
	IDT[_idt].zero = 0;
	IDT[_idt].type_attr = type_flags;
	IDT[_idt].bits_offset_higher = (irq_addr & 0xFFFF0000) >> 16;
}

void set_idt_values(uint8_t _idt, uint32_t irq_addr) {
	idt_install_interrupt_request(_idt, irq_addr, 0x8E);
}

//Duplicate, but other functions use this version. Fix in the future
void idt_new_int(uint8_t inum, uint32_t irq_function) {
	set_idt_values(inum,irq_function);
}

void idt_new_int_flags(uint8_t inum, uint32_t irq_function, uint16_t type_attrs) {
	idt_install_interrupt_request(inum, irq_function, 0x8E | type_attrs);
}

void unhandled_interrupt() {
	asm("cli");
	kerror("[IDT] Unhandled interupt");
	for (;;);
}

void (*irq_functions[16][16])() = {0};

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
	
	//Initialize all interrupts with default handler
	uint32_t default_handler_addr = (uint32_t)default_handler;
	for (uint16_t i = 0; i < 16; i++) {
		set_idt_values(i,default_handler_addr);
	}
	
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
	
	init_exceptions();
	
	idt_addr = (uint32_t)IDT;
	idt_ptr[0] = (sizeof (struct IDT_entry_t) * 256) + ((idt_addr & 0xffff) << 16);
	idt_ptr[1] = idt_addr >> 16;
	
	load_idt(idt_ptr);
	
	kprint("[INIT] IDT Enabled");
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

void execute_irq(uint8_t irq) {
	for (uint8_t i = 0; i < 16; i++) {
		if (irq_functions[irq][i]) {
			irq_functions[irq][i]();
		}
	}
	irq_finished[irq] = true;
}

void add_irq_function(uint8_t irq, void (*function)()) {
	for (uint8_t i = 0; i < 16; i++) {
		if (!irq_functions[irq][i]) {
			if (irq_functions[irq][i]==function)
				break;
			irq_functions[irq][i] = function;
			break;
		}
	}
}

/*void set_irq_finish_state(uint8_t irq, _Bool state) {
	irq_finished[irq] = state;
}*/

_Bool ts_enabled = false;
tss_entry_t temp_tss;
uint32_t ready_esp;

void irq0_handler(void) {
	outb(0x20, 0x20); //EOI
	if (ts_enabled)
		asm("nop"); //Debug breakpoint
	pit_tick();
	usb_keyboard_repeat();
	if (ts_enabled&&!(get_ticks()%10))
		task_switch(temp_tss,ready_esp);
	execute_irq(0);
}
 
void irq1_handler(void) {
	outb(0x20, 0x20); //EOI
	kbd_handler();
	execute_irq(1);
}
 
void irq2_handler(void) {
	outb(0x20, 0x20); //EOI
	execute_irq(2);
}
 
void irq3_handler(void) {
	outb(0x20, 0x20); //EOI
	execute_irq(3);
}
 
void irq4_handler(void) {
	outb(0x20, 0x20); //EOI
	execute_irq(4);
}
 
void irq5_handler(void) {
	outb(0x20, 0x20); //EOI
	execute_irq(5);
}
 
void irq6_handler(void) {
	outb(0x20, 0x20); //EOI
	execute_irq(6);
}
 
void irq7_handler(void) {
	outb(0x20, 0x20); //EOI
	execute_irq(7);
}
 
void irq8_handler(void) {
	outb(0xA0, 0x20);
	outb(0x20, 0x20); //EOI          
	execute_irq(8);
}
 
void irq9_handler(void) {
	outb(0xA0, 0x20);
	outb(0x20, 0x20); //EOI
	execute_irq(9);
}
 
void irq10_handler(void) {
	outb(0xA0, 0x20);
	outb(0x20, 0x20); //EOI
	execute_irq(10);
}
 
void irq11_handler(void) {
	outb(0xA0, 0x20);
	outb(0x20, 0x20); //EOI
	execute_irq(11);
}

void irq12_handler(void) {
	outb(0xA0, 0x20);
	outb(0x20, 0x20); //EOI
	mouse_handler();
	execute_irq(12);
}
 
void irq13_handler(void) {
	outb(0xA0, 0x20);
	outb(0x20, 0x20); //EOI
	kerror("[Exception.Interrupt] FPU Error Interrupt!");
	execute_irq(13);
}
 
void irq14_handler(void) {
	outb(0xA0, 0x20);
	outb(0x20, 0x20); //EOI
	execute_irq(14);
}
 
void irq15_handler(void) {
	outb(0xA0, 0x20);
	outb(0x20, 0x20); //EOI
	execute_irq(15);
}

void enable_tasking() {
	ts_enabled = true;
}

void disable_tasking() {
	ts_enabled = false;
}