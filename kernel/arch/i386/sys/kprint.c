#include <kernel/kprint.h>
#include <stdarg.h>

/* void panic(const char* str) {
	kerror("KERNEL PANIC:");
	kerror(str);
	abort();
}
 */
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
