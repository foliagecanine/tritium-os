#ifndef _GFUNC_H
#define _GFUNC_H

#include <stdio.h>
#include <sys.h>

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

extern uint16_t framebuffer_width;
extern uint16_t framebuffer_height;
extern uint32_t framebuffer_pitch;

#define g_width framebuffer_width
#define g_height framebuffer_height
#define g_pitch framebuffer_pitch

typedef struct {
	uint8_t b;
	uint8_t g;
	uint8_t r;
} __attribute__((packed)) color24;

typedef struct {
	uint8_t b;
	uint8_t g;
	uint8_t r;
	uint8_t a;
} __attribute__((packed)) color32;

typedef color32 color_t;

typedef struct {
	int width;
	int height;
	int pitch;
	int bpp;
	void *framebuffer;
} graphics_mode_t;

int claim_graphics(void);
int release_graphics(void);
int set_resolution(uint16_t width, uint16_t height, uint8_t bpp);
int set_text_mode(void);
int drawframe(void);
color_t getpixel(uint32_t x, uint32_t y);
void setpixel(uint32_t x, uint32_t y, color_t c);
void setpixel_a(uint32_t x, uint32_t y, color_t c);
void rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, color_t c);
void rect_a(uint32_t x, uint32_t y, uint32_t w, uint32_t h, color_t c);
graphics_mode_t get_graphics_mode(void);

#endif
