#include <tty.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t terminal_buffer[VGA_WIDTH*VGA_HEIGHT];
bool scroll = true;

// 0 = Set Cursor Position
// 1 = Get Cursor Position: 0x0000XXYY
uint32_t terminal_options(uint8_t command,uint8_t x, uint8_t y) {
	uint32_t retval;
	asm volatile("pusha;\
				movb %1,%%bl;\
				movb %2,%%cl; \
				movb %3,%%dl;\
				mov $9,%%eax;\
				int $0x80;\
				mov %%eax,%0;\
				popa":"=m"(retval):"m"(command),"m"(x),"m"(y));
	return retval;
}

void terminal_getcursor() {
	uint32_t o = terminal_options(1,0,0);
	terminal_row = o&0xFF;
	terminal_column = (o>>8)&0xFF;
}

void terminal_setcursor() {
	terminal_options(0,(uint8_t)terminal_column,(uint8_t)terminal_row);
}

void terminal_init() {
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);;
	terminal_clear();
	terminal_setcursor();
}

void terminal_setcolor(uint8_t color) {
	terminal_color = color;
}

uint8_t terminal_getcolor() {
	return terminal_color;
}

void terminal_putentryat(unsigned char c, unsigned char color, unsigned int x, unsigned int y) {
	asm volatile("pusha;\
				movb %0,%%bl;\
				movb %1,%%cl; \
				mov %2,%%edx;\
				mov %3,%%esi;\
				mov $3,%%eax;\
				int $0x80;\
				popa"::"m"(c),"m"(color),"m"(x),"m"(y));
	terminal_buffer[(VGA_WIDTH*y)+x] = vga_entry(c,color);
}

void terminal_refresh() {
	for (size_t y=0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			terminal_putentryat(terminal_buffer[index], terminal_color, x, y);
		}
	}
}

void terminal_scroll() {
	terminal_options(2,0,0);
}

void terminal_putchar(char c) {
	terminal_getcursor();
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
	terminal_setcursor();
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

void terminal_backup() {
	terminal_getcursor();
	if (terminal_column>0) {
		terminal_column--;
	} else {
		terminal_row--;
		terminal_column = VGA_WIDTH-1;
	}
	terminal_setcursor();
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
  terminal_setcursor();
}

void exit(uint32_t code) {
	asm("mov $2,%eax;int $0x80");
}