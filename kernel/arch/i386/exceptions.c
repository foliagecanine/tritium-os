#include <kernel/exceptions.h>

//List all exceptions from https://wiki.osdev.org/Exceptions

void exception_div_by_zero() {
	kerror("[Exception.Fault] Divide by zero!");
	for (;;);
}

void exception_debug() {
	kerror("[Exception.Trap] Debug!");
	for (;;);
}

void exception_nmi() {
	kerror("[Exception.Interrupt] Non Maskable Interrupt!");
	for (;;);
}

void exception_breakpoint() {
	kerror("[Exception.Trap] Breakpoint");
	for (;;);
}

void exception_overflow() {
	kerror("[Exception.Trap] Overflow");
	for (;;);
}

void exception_bound_range_exceeded() {
	kerror("[Exception.Fault] Bound Range Exceeded!");
	for (;;);
}

void exception_invalid_opcode() {
	kerror("[Exception.Fault] Invalid Opcode!");
	for (;;);
}

void exception_device_not_available() {
	kerror("[Exception.Fault] Device Not Available!");
	for (;;);
}

void exception_double_fault() {
	kerror("[Exception.Abort] Double Fault!");
	for (;;);
}

void exception_coprocessor_segment_overrun() {
	kerror("[Exception.Fault] Coprocessor Segment Overrun!");
	for (;;);
}

void exception_invalid_tss() {
	kerror("[Exception.Fault] Invalid TSS!");
	for (;;);
}

void exception_segment_not_present() {
	kerror("[Exception.Fault] Segment Not Present!");
	for (;;);
}

void exception_stack_segment_fault() {
	kerror("[Exception.Fault] Stack-Segment Fault!");
	for (;;);
}

void exception_general_protection_fault() {
	kerror("[Exception.Fault] General Protection Fault!");
	printf("Current PID: %u\n",getpid());
	dprintf("Current PID: %$\n",getpid());
	for (;;);
}

uint32_t address;

void exception_page_fault(uint32_t retaddr, uint32_t error) {
	disable_tasking();
	asm volatile("mov %%cr2, %0":"=a"(address):);
	terminal_setcolor(0x1F);
	terminal_refresh();
	if(error&4) {
		//Try to correct the error by giving it a blank page. 
		//We'll clear this so it doesn't see any other process data.
		kwarn("[WARN] Pre-Pagefault: Attempt to access unavailable page.");
		if (!map_page_to((void *)(address&0xFFFFF000))) {
			kerror("[Exception.Fault] Usermode Page Privelage Fault!");
			printf("Fault address: %p\n",address);
			printf("Return address: %p\n",retaddr);
			printf("Current PID: %u\n",getpid());
			printf("Error: 0x%X\n",(error&7) | (error&0x10));
			printf("Permissions: 0x%X\n",get_page_permissions((void *)address));
			dprintf("Fault address: 0x%X\n",address);
			dprintf("Return address: 0x%X\n",retaddr);
			dprintf("Current PID: %u\n",getpid());
			dprintf("Error: 0x%X\n",error&7 | error&0x10);
			dprintf("Permissions: 0x%X\n",get_page_permissions((void *)address));
			kprint("\n    If you are a user seeing this, your computer has crashed.");
			kprint("    Reboot your computer. If the error happens again...");
			kprint("    try to not do the thing that made it happen.\n");
			abort();
			for(;;);
		} else {
			mark_user((void *)(address&0xFFFFF000),true);
			kwarn("[WARN] A page was given to a process after a page fault.");
			dprintf("Access attempted: %p\n",address);
		}
		//asm("mov %0,%%esp; popa; mov %1,%%esp; add $4,%%esp; iret"::"m"(temp_post_esp),"m"(a_err));
		return;
	} else {
		kerror("[Exception.Fault] Kernel Page Fault!");
		printf("Fault address: %p\n",address);
		printf("Return address: %p\n",retaddr);
		printf("Error: 0x%X\n",error&7 | error&0x10);
		printf("Permissions: 0x%X\n",get_page_permissions((void *)address));
		dprintf("Fault address: 0x%X\n",address);
		dprintf("Return address: 0x%X\n",retaddr);
		dprintf("Error: 0x%X\n",error&7 | error&0x10);
		dprintf("Permissions: 0x%X\n",get_page_permissions((void *)address));
		kprint("\nIf you are a user seeing this, your computer has crashed.");
		kprint("Reboot your computer. If the error happens again...");
		kprint("just don't to the thing that made it happen.\n");
		abort();
		for(;;);
	}
}

//Interrupt 15 reserved here

void exception_x87_floating_point_exception() {
	kerror("[Exception.Fault] x87 Floating Point Exception!");
	for (;;);
}

void exception_alignment_check() {
	kerror("[Exception.Fault] Alignment Check!");
	for (;;);
}

void exception_machine_check() {
	kerror("[Exception.Abort] Machine Check!");
	for (;;);
}

void exception_simd_floating_point_exception() {
	kerror("[Exception.Fault] SIMD Floating-Point Exception!");
	for (;;);
}

void exception_virtualization_exception() {
	kerror("[Exception.Fault] Virtualization Exception!");
	for (;;);
}

//Interrupts 21-29 reserved here

void exception_security_exception() {
	kerror("[Exception.General] Security Exception!");
	for (;;);
}

extern void page_fault();

void init_exceptions() {
	uint32_t exception_addr = (uint32_t)exception_div_by_zero;
	idt_new_int(0, exception_addr);
	
	exception_addr = (uint32_t)exception_debug;
	idt_new_int(1, exception_addr);
	
	exception_addr = (uint32_t)exception_nmi;
	idt_new_int(2, exception_addr);
	
	exception_addr = (uint32_t)exception_breakpoint;
	idt_new_int(3, exception_addr);
	
	exception_addr = (uint32_t)exception_overflow;
	idt_new_int(4, exception_addr);
	
	exception_addr = (uint32_t)exception_bound_range_exceeded;
	idt_new_int(5, exception_addr);
	
	exception_addr = (uint32_t)exception_invalid_opcode;
	idt_new_int(6, exception_addr);
	
	exception_addr = (uint32_t)exception_device_not_available;
	idt_new_int(7, exception_addr);
	
	exception_addr = (uint32_t)exception_double_fault;
	idt_new_int(8, exception_addr);
	
	exception_addr = (uint32_t)exception_coprocessor_segment_overrun;
	idt_new_int(9, exception_addr);
	
	exception_addr = (uint32_t)exception_invalid_tss;
	idt_new_int(10, exception_addr);
	
	exception_addr = (uint32_t)exception_segment_not_present;
	idt_new_int(11, exception_addr);
	
	exception_addr = (uint32_t)exception_stack_segment_fault;
	idt_new_int(12, exception_addr);
	
	exception_addr = (uint32_t)exception_general_protection_fault;
	idt_new_int(13, exception_addr);
	
	exception_addr = (uint32_t)page_fault;
	idt_new_int(14, exception_addr);
	
	exception_addr = (uint32_t)exception_x87_floating_point_exception;
	idt_new_int(16, exception_addr);
	
	exception_addr = (uint32_t)exception_alignment_check;
	idt_new_int(17, exception_addr);
	
	exception_addr = (uint32_t)exception_machine_check;
	idt_new_int(18, exception_addr);
	
	exception_addr = (uint32_t)exception_simd_floating_point_exception;
	idt_new_int(19, exception_addr);
	
	exception_addr = (uint32_t)exception_virtualization_exception;
	idt_new_int(20, exception_addr);
	
	exception_addr = (uint32_t)exception_security_exception;
	idt_new_int(30, exception_addr);
}