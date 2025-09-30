#include <kernel/pit.h>
#include <kernel/sysfunc.h>

// Thanks to http://jamesmolloy.co.uk/tutorial_html/5.-IRQs%20and%20the%20PIT.html
// for clarification of the assembly code from https://wiki.osdev.org/PIT
uint32_t frequency = 0;
uint64_t ticks = 0;

void pit_tick() {
	ticks++;
}

uint32_t get_ticks(bool high) {
	if (high)
		return (uint32_t)(ticks >> 32);
	return (uint32_t)ticks;
}

uint64_t get_ticks_k() {
	return ticks;
}

void init_pit(uint32_t freq) {
	frequency = freq;
	uint32_t pitfreq = 1193181 / freq;
	outb(0x43, 0x34);
	outb(0x40, (uint8_t)(pitfreq));
	outb(0x40, (uint8_t)(pitfreq >> 8));
}

void sleep(uint32_t ms) {
	uint64_t endTicks = ticks + ((ms * frequency) / 1000);
	while (ticks < endTicks) {
		asm volatile("nop");
	}
}
