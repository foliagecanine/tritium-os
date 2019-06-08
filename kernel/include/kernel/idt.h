#ifndef _KERNEL_IDT_H
#define _KERNEL_IDT_H

#include <kernel/stdio.h>
#include <kernel/kbd.h>

void init_idt();
void clear_irq_status(uint8_t irq);
_Bool has_irq_finished(uint8_t irq);
void set_irq_finish_state(uint8_t irq, _Bool state);

#endif