#ifndef _KERNEL_GRAPHICS_H
#define _KERNEL_GRAPHICS_H

#include <kernel/stdio.h>
#include <kernel/task.h>
#include <kernel/ksetup.h>

#define GRAPHICS_ERR_NO_LOCK        1
#define GRAPHICS_ERR_INVALID_BPP    2
#define GRAPHICS_ERR_NO_FRAMEBUFFER 3
#define GRAPHICS_ERR_MODE_SET_FAIL  4

typedef union graphics_mode {
    struct {
        uint16_t width:12;
        uint16_t height:12;
        uint8_t bpp:8;
    } __attribute__((packed));
    uint32_t raw;
} graphics_mode;

bool has_graphics_lock(uint32_t pid);
uint32_t request_graphics_lock(void);
uint32_t release_graphics_lock(void);
graphics_mode set_resolution(uint32_t width, uint32_t height, uint8_t bits);
uint32_t set_text_mode(void);
uint32_t copy_framebuffer(void *addr);
uint32_t get_framebuffer_size(void);

#endif