#include "gfunc.h"

#define GRAPHICS_FUNCTION 27
#define GRAPHICS_FUNCTION_LOCK 0
#define GRAPHICS_FUNCTION_UNLOCK 1
#define GRAPHICS_FUNCTION_SETRES 2
#define GRAPHICS_FUNCTION_COPY 3
#define GRAPHICS_FUNCTION_SETTEXT 4

uint16_t framebuffer_width = 0;
uint16_t framebuffer_height = 0;
uint8_t  framebuffer_bpp = 0;
uint32_t framebuffer_pitch = 0;
void *	 framebuffer_alloc = 0;
void *   framebuffer = 0;

int claim_graphics() { return _syscall1(GRAPHICS_FUNCTION, GRAPHICS_FUNCTION_LOCK); }

int release_graphics() { return _syscall1(GRAPHICS_FUNCTION, GRAPHICS_FUNCTION_UNLOCK); }

int set_resolution(uint16_t width, uint16_t height, uint8_t bpp) {
	uint32_t retval = _syscall4(GRAPHICS_FUNCTION, GRAPHICS_FUNCTION_SETRES, width, height, bpp);
	graphics_mode graphics;
	graphics.raw = retval;
	framebuffer_bpp = graphics.bpp;
	framebuffer_height = graphics.height;
	framebuffer_width = graphics.width;
	framebuffer_pitch = framebuffer_width * framebuffer_bpp / 8;
	if (framebuffer_width == 0)
		return framebuffer_bpp;
	framebuffer_alloc = malloc(framebuffer_pitch * framebuffer_height + sizeof(void *));
	if (!framebuffer_alloc)
		return 0xEE;

	// Align to 16b for best performance
	framebuffer = (framebuffer_alloc + 16) - ((size_t)framebuffer_alloc % 16);
	return 0;
}

int set_text_mode() {
	if (framebuffer_alloc) {
		free(framebuffer_alloc);
		framebuffer_alloc = 0;
		framebuffer = 0;
	}
	framebuffer_width = 0;
	framebuffer_height = 0;
	framebuffer_bpp = 0;
	framebuffer_pitch = 0;

	return _syscall1(GRAPHICS_FUNCTION, GRAPHICS_FUNCTION_SETTEXT);
}

graphics_mode_t get_graphics_mode() {
	return (graphics_mode_t){.width = framebuffer_width, .height = framebuffer_height, .pitch = framebuffer_pitch, .bpp = framebuffer_bpp, .framebuffer = framebuffer};
}

int drawframe() { return _syscall2(GRAPHICS_FUNCTION, GRAPHICS_FUNCTION_COPY, (uintptr_t)framebuffer); }

color24 getpixel24(uint32_t x, uint32_t y) {
	if (x < framebuffer_width && y < framebuffer_height)
		return ((color24 *)framebuffer)[(framebuffer_width * y) + x];
}

color32 getpixel32(uint32_t x, uint32_t y) {
	if (x < framebuffer_width && y < framebuffer_height)
		return ((color32 *)framebuffer)[(framebuffer_width * y) + x];
}

color_t getpixel(uint32_t x, uint32_t y) {
	if (framebuffer_bpp == 24) {
		color24 c = getpixel24(x, y);
		return (color_t){c.r, c.g, c.b, 255};
	} else {
		return getpixel32(x, y);
	}
}

void setpixel24(uint32_t x, uint32_t y, color24 c) {
	if (x < framebuffer_width && y < framebuffer_height)
		((color24 *)framebuffer)[(framebuffer_width * y) + x] = c;
}

void setpixel32(uint32_t x, uint32_t y, color32 c) {
	if (x < framebuffer_width && y < framebuffer_height) {
		((color32 *)framebuffer)[(framebuffer_width * y) + x] = c;
	}
}

void setpixel(uint32_t x, uint32_t y, color_t c) {
	if (framebuffer_bpp == 24) {
		setpixel24(x, y, (color24){c.r, c.g, c.b});
	} else {
		setpixel32(x, y, c);
	}
}

int over_operator(int alpha, int colora, int colorb) {
	return ((colora * alpha) + (colorb * (255 - alpha))) >> 8;
}

void setpixel_a(uint32_t x, uint32_t y, color_t c) {
	if (c.a == 0)
		return;
	if (c.a == 255) {
		setpixel(x, y, c);
		return;
	}

	color_t under_pixel = getpixel(x, y);
	color_t new_color;
	new_color.r = over_operator(c.a, c.r, under_pixel.r);
	new_color.g = over_operator(c.a, c.g, under_pixel.g);
	new_color.b = over_operator(c.a, c.b, under_pixel.b);
	new_color.a = 0;

	if (framebuffer_bpp == 24) {
		setpixel24(x, y, (color24){new_color.r, new_color.g, new_color.b});
	} else {
		setpixel32(x, y, new_color);
	}
}

void rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, color_t c) {
	if (framebuffer_bpp == 24) {
		color24 c24 = (color24){c.r, c.g, c.b};
		for (uint32_t _y = y; _y < y + h; _y++) {
			for (uint32_t _x = x; _x < x + w; _x++) {
				((color24 *)framebuffer)[(framebuffer_width * _y) + _x] = c24;
			}
		}
	} else {
		for (uint32_t _y = y; _y < y + h; _y++) {
			for (uint32_t _x = x; _x < x + w; _x++) {
				((color32 *)framebuffer)[(framebuffer_width * _y) + _x] = (color32)c;
			}
		}
	}
}

void rect_a(uint32_t x, uint32_t y, uint32_t w, uint32_t h, color_t c) {
	for (uint32_t _y = y; _y < y + h; _y++) {
		for (uint32_t _x = x; _x < x + w; _x++) {
			setpixel_a(_x, _y, c);
		}
	}
}
