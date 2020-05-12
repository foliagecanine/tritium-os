#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <kernel/tty.h>

#include "vga.h"

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
static uint16_t* const VGA_MEMORY = (uint16_t*) 0xC03FF000;

static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;
static uint16_t* terminal_buffer;
static bool scroll = true;

void terminal_initialize(void) {
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	terminal_buffer = VGA_MEMORY;
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}
}

void terminal_setcolor(uint8_t color) {
	terminal_color = color;
}

uint8_t terminal_getcolor() {
	return terminal_color;
}

void terminal_putentryat(unsigned char c, uint8_t color, size_t x, size_t y) {
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);
}

void terminal_scroll() {
	if (scroll) {
		for (size_t y = 1; y < VGA_HEIGHT; y++) {
			for (size_t x = 0; x < VGA_WIDTH; x++) {
				size_t from_index = y * VGA_WIDTH + x;
				size_t to_index = (y-1) * VGA_WIDTH + x;
				terminal_buffer[to_index] = terminal_buffer[from_index];
			}
		}
		for (size_t x = 0; x < VGA_WIDTH; x++)
			terminal_buffer[(VGA_HEIGHT-1)*VGA_WIDTH+x] = vga_entry(' ', terminal_color);
	}
}

void terminal_putchar(char c) {
	unsigned char uc = c;
	if (c!='\n')
		terminal_putentryat(uc, terminal_color, terminal_column, terminal_row);
	if (++terminal_column == VGA_WIDTH || c == '\n') {
		terminal_column = 0;
		if (++terminal_row == VGA_HEIGHT) {
			terminal_row--;
			terminal_scroll();
		}
	}
}

void terminal_write(const char* data, size_t size) {
	for (size_t i = 0; i < size; i++)
		terminal_putchar(data[i]);
}

void terminal_writestring(const char* data) {
	terminal_write(data, strlen(data));
}

void set_scroll(_Bool allow_scroll) {
	scroll = allow_scroll;
}

void disable_cursor() {
	outb(0x3D4,0x0A);
	outb(0x3D5,0x20);
}

void cursor_position(size_t x, size_t y) {
	size_t index = y * VGA_WIDTH + x;
	outb(0x3D4,0x0F);
	outb(0x3D5, (uint8_t) (index & 0xFF));
	outb(0x3D4,0x0E);
	outb(0x3D5, (uint8_t) ((index>>8) & 0xFF));
	terminal_column = x;
	terminal_row = y;
}

void enable_cursor(uint8_t start, uint8_t end) {
	outb(0x3D4, 0x0A);
	outb(0x3D5, (inb(0x3D5) & 0xC0) | start);
	outb(0x3D4, 0x0B);
	outb(0x3D5, (inb(0x3D5) & 0xE0) | end);
}

void terminal_backup() {
	if (terminal_column>0) {
		terminal_column--;
	} else {
		terminal_row--;
		terminal_column = VGA_WIDTH-1;
	}
}

void terminal_clear_line(size_t y)   //clear given line
{
  for(size_t x=0; x<VGA_WIDTH; x++)
    terminal_putentryat(' ', terminal_color, x, y);
}

void terminal_clear(void)    //clear all screen and set prompt to up left corner
{
  for(size_t y=0; y<VGA_HEIGHT; y++)
	terminal_clear_line(y);
  terminal_row = 0;
  terminal_column = 0;
}

void terminal_refresh() {
	for (size_t y=0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			terminal_putentryat(terminal_buffer[index], terminal_color, x, y);
		}
	}
}
