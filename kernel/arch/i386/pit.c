#include <kernel/pit.h>
#include <kernel/sysfunc.h>

// Thanks to http://jamesmolloy.co.uk/tutorial_html/5.-IRQs%20and%20the%20PIT.html 
// for clarification of the assembly code from https://wiki.osdev.org/PIT
uint32_t frequency = 0;
uint64_t ticks = 0;

void pit_tick() {
	ticks++;
}

uint64_t get_ticks() {
	return ticks;
}

void init_pit(uint32_t freq) {
	frequency = freq;
	uint32_t pitfreq = 3579545/ (freq*3);
	outb(0x43, 0x36);
	outb(0x40, (uint8_t)(pitfreq&0xFF));
	outb(0x40, (uint8_t)((pitfreq>>8)&0xFF));
}

void dontoptimize() {
	asm volatile("nop");
}

void sleep(uint32_t ms) {
	uint64_t endTicks = ticks+((ms*frequency)/1000);
	while (ticks<endTicks) {
		//printf("Ticks left: %d\n",endTicks-ticks);
		dontoptimize(); // It doesn't like if we don't have something here
	}
}
