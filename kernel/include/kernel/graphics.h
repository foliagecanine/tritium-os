#ifndef _KERNEL_GRAPHICS_H
#define _KERNEL_GRAPHICS_H

#include <kernel/stdio.h>
#include <kernel/task.h>
#include <kernel/ksetup.h>

uint32_t request_graphics_lock(void);
uint32_t request_graphics_unlock(void);
uint32_t set_resolution(uint32_t width, uint32_t height, uint8_t bits);
uint32_t copy_framebuffer(void *addr);
uint32_t get_framebuffer_size(void);

#endif