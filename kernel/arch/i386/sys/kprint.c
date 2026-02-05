#include <kernel/kprint.h>
#include <kernel/serial.h>
#include <kernel/multiboot.h>
#include <kernel/elf.h>
#include <string.h>

/* void panic(const char* str) {
	kerror("KERNEL PANIC:");
	kerror(str);
	abort();
}
 */

typedef struct stack_frame {
	struct stack_frame* ebp;
	uint32_t eip;
} stack_frame_t;

void dump_stacktrace() {
	stack_frame_t* frame;
	asm volatile("mov %%ebp, %0" : "=r"(frame));
	kprint("Stack trace:");
	
	dprintf("Frame at %p\n", frame);
	uint8_t perms = get_page_permissions((void*)frame);
	if (perms == PAGE_ERROR_NOT_PRESENT) {
		kprintf("  (Unavailable)\n");
		return;
	}

	while (frame) {
		perms = get_page_permissions((void*)frame->eip);
		if (perms == PAGE_ERROR_NOT_PRESENT) {
			kprintf("  at 0x%08X (unmapped)\n", frame->eip);
			break;
		}

		kprintf("  at 0x%08X\n", frame->eip);

		perms = get_page_permissions((void*)frame->ebp);
		if (perms == PAGE_ERROR_NOT_PRESENT) {
			kprintf("  next frame at 0x%08X (unmapped)\n", (uint32_t)(uintptr_t)frame->ebp);
			break;
		}

		frame = frame->ebp;
	}
}

void kpanic(const char* str, const char* file, int line) {
	uint8_t prev_trm_color = terminal_getcolor();
	terminal_setcolor((prev_trm_color|0xc)&0xfc);
	printf("\nKERNEL PANIC: %s: %s:%d\n",str,file,line);
	dprintf("\nKERNEL PANIC: %s: %s:%d\n",str,file,line);
	terminal_setcolor(prev_trm_color);

	dump_stacktrace();

	abort();
}

void kerror(const char* str) {
	uint8_t prev_trm_color = terminal_getcolor();
	terminal_setcolor((prev_trm_color|0xc)&0xfc);
	printf(str);
	printf("\n");
	dprintf(str);
	dprintf("\n");
	terminal_setcolor(prev_trm_color);
}

void kprint(const char* str) {
	uint8_t prev_trm_color = terminal_getcolor();
	terminal_setcolor((prev_trm_color|0xe)&0xfe);
	printf(str);
	printf("\n");
	dprintf(str);
	dprintf("\n");
	terminal_setcolor(prev_trm_color);
}

void kwarn(const char* str) {
	uint8_t prev_trm_color = terminal_getcolor();
	terminal_setcolor((prev_trm_color|0x5)&0xf5);
	printf(str);
	printf("\n");
	dprintf(str);
	dprintf("\n");
	terminal_setcolor(prev_trm_color);
}

int kprintf(const char* format, ...) {
	va_list args;
	va_start(args, format);

	// Buffer to hold the formatted string
	char buffer[1024];
	int length = vsnprintf(buffer, sizeof(buffer), format, args);

	// Print to terminal
	if (length > 0) {
		uint8_t prev_trm_color = terminal_getcolor();
		terminal_setcolor((prev_trm_color|0xe)&0xfe);
		print(buffer, length);
		serial_write(buffer);
		terminal_setcolor(prev_trm_color);
	}

	va_end(args);

	return length;
}