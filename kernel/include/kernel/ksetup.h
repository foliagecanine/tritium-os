#ifndef _KERNEL_KSETUP_H
#define _KERNEL_KSETUP_H

#include <kernel/stdio.h>
#include <kernel/idt.h>
#include <kernel/pit.h>
#include <kernel/kbd.h>
#include <kernel/multiboot.h>
#include <kernel/tss.h>
#include <stdbool.h>
#include <kernel/acpi.h>

void init_gdt();
void install_tss();
void power_shutdown();
void power_reboot();
void kernel_exit();

#endif
