#include <stdio.h>
#include <string.h>
#include <tty.h>
#include <gui.h>

void drawrect(size_t x, size_t y, size_t w, size_t h, uint8_t color) {
	for (uint8_t _y = y; _y < y + h; _y++) {
	terminal_goto(x,_y);
		for (uint8_t _x = x; _x < x + w; _x++) {
			terminal_putentryat(' ',color,_x,_y);
		}
	}
}
