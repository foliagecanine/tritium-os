#include <kernel/exceptions.h>
#include <kernel/mem.h>
#include <kernel/kprint.h>

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

	dump_stacktrace();
	for (;;);
}

void *address;

void dump_page_tables();

const char *page_fault_reasons[] = {
	"",
	"Write",
	"User",
	"Reserved Write",
	"Instruction Fetch",
	"Protection Key Violation",
	"Shadow Stack Access",
	"SGX Access"
};

const char *page_fault_inverse_reasons[] = {
	"Not Present",
	"Read",
	"Kernel",
	"",
	"",
	"",
	"",
	""
};

void exception_page_fault(void *retaddr, uint32_t error) {
	disable_tasking();
	asm volatile("mov %%cr2, %0" : "=a" (address) :);
	if(error & 4) {
		kerror("[Exception.Fault] Usermode Page Privelage Fault!");
		uint8_t page_permissions = get_page_permissions(address);
		printf("Fault address: %p\n", address);
		printf("Return address: %p\n", retaddr);
		printf("Current PID: %lu\n", getpid());
		
		printf("Error:");
		for (int i = 0; i < 8; i++) {
			if (error & (1 << i)) {
				printf(" %s", page_fault_reasons[i]);
			} else {
				printf(" %s", page_fault_inverse_reasons[i]);
			}
		}
		printf("\n");

		printf("Permissions:");

		if (page_permissions == PAGE_ERROR_NOT_PRESENT) {
			printf(" Page not present");
		} else {
			if(page_permissions & PAGE_PERM_USER) {
				printf(" User");
			} else {
				printf(" Kernel");
			}

			if(page_permissions & PAGE_PERM_WRITE) {
				printf(" Write");
			} else {
				printf(" Read");
			}
		}

		printf("\n");

		dprintf("Fault address: %p\n", address);
		dprintf("Return address: %p\n", retaddr);
		dprintf("Current PID: %lu\n", getpid());
		dprintf("Error:");
		for (int i = 0; i < 8; i++) {
			if (error & (1 << i)) {
				dprintf(" %s", page_fault_reasons[i]);
			} else {
				dprintf(" %s", page_fault_inverse_reasons[i]);
			}
		}
		dprintf("\n");
		dprintf("Permissions:");

		if(page_permissions & PAGE_PERM_USER) {
			dprintf(" User");
		} else {
			dprintf(" Kernel");
		}

		if(page_permissions & PAGE_PERM_WRITE) {
			dprintf(" Write");
		} else {
			dprintf(" Read");
		}

		dprintf("\n");

		dump_stacktrace();
		
		dump_page_tables();

		exit_program(1);
		// Should not reach here.
		for(;;);
		return;
	} else {
		terminal_setcolor(0x1F);
		terminal_refresh();
		kerror("[Exception.Fault] Kernel Page Fault!");
		uint8_t page_permissions = get_page_permissions(address);
		printf("Fault address: %p\n", address);
		printf("Return address: %p\n", retaddr);
		printf("Error:");
		for (int i = 0; i < 8; i++) {
			if (error & (1 << i)) {
				printf(" %s", page_fault_reasons[i]);
			} else {
				printf(" %s", page_fault_inverse_reasons[i]);
			}
		}
		printf("\n");
		printf("Permissions:");

		if(page_permissions & PAGE_PERM_USER) {
			printf(" User");
		} else {
			printf(" Kernel");
		}

		if(page_permissions & PAGE_PERM_WRITE) {
			printf(" Write");
		} else {
			printf(" Read");
		}

		printf("\n");

		dprintf("Fault address: %p\n", address);
		dprintf("Return address: %p\n", retaddr);
		dprintf("Error:");
		for (int i = 0; i < 8; i++) {
			if (error & (1 << i)) {
				dprintf(" %s", page_fault_reasons[i]);
			} else {
				dprintf(" %s", page_fault_inverse_reasons[i]);
			}
		}
		dprintf("\n");
		dprintf("Permissions:");

		if (page_permissions == PAGE_ERROR_NOT_PRESENT) {
			dprintf(" Page not present");
		} else {
			if(page_permissions & PAGE_PERM_USER) {
				dprintf(" User");
			} else {
				dprintf(" Kernel");
			}
	
			if(page_permissions & PAGE_PERM_WRITE) {
				dprintf(" Write");
			} else {
				dprintf(" Read");
			}
		}

		dprintf("\n");

		kprint("\nIf you are a user seeing this, your computer has crashed.");
		kprint("Reboot your computer. If the error happens again...");
		kprint("just don't do the thing that made it happen.\n");
		
		dump_stacktrace();

		dump_page_tables();
		
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