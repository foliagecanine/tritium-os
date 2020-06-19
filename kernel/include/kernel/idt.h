#ifndef _KERNEL_IDT_H
#define _KERNEL_IDT_H

#include <kernel/stdio.h>
#include <kernel/kbd.h>
#include <kernel/exceptions.h>
#include <kernel/pit.h>
#include <kernel/task.h>

void init_idt();
void clear_irq_status(uint8_t irq);
_Bool has_irq_finished(uint8_t irq);
void set_irq_finish_state(uint8_t irq, _Bool state);
void idt_new_int(uint8_t inum, uint32_t irq_function);
void idt_new_int_flags(uint8_t inum, uint32_t irq_function, uint16_t type_attrs);
void enable_tasking();
void disable_tasking();

#endif
