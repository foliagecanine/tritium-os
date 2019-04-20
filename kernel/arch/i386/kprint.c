#include <kernel/kprint.h>

void panic(const char* str) {
	kerror("KERNEL PANIC:");
	kerror(str);
	abort();
}

void kerror(const char* str) {
	uint8_t prev_trm_color = terminal_getcolor();
	terminal_setcolor(0xc,0x0);
	printf(str);
	printf("\n");
	terminal_setcolor(prev_trm_color);
}

void kprint(const char* str) {
	uint8_t prev_trm_color = terminal_getcolor();
	terminal_setcolor(0xe,0x0);
	printf(str);
	printf("\n");
	terminal_setcolor(prev_trm_color);
}
