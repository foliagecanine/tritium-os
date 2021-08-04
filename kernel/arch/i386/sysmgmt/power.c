#include <kernel/ksetup.h>

void power_reboot() {
    while (inb(0x64) & 0x02)
		;
    outb(0x64, 0xFE);
    kerror("Failed to reboot. Please manually reboot computer.");
	abort();
}

void power_shutdown() {
	kprint("It is safe to turn off your computer.");
	kprint("[KMSG] Attempting shutdown using ACPI.");
	acpi_shutdown();
	sleep(1000);
	kerror("ACPI shutdown failed.");
	extern void bios32();
	identity_map((void *)0x7000);
	identity_map((void *)0x8000);
	kprint("[KMSG] Attempting shutdown using BIOS.");
	asm("\
	mov $0x1553,%ax;\
	call bios32;\
	");
	kerror("BIOS shutdown failed.");
	asm("\
	1:cli;\
	hlt;\
	jmp 1\
	");
	for(;;);
}