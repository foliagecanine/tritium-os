#ifndef _STDIO_H
#define _STDIO_H 1

//#include <sys/cdefs.h>
#include <stdint.h>
#include <sys.h>

#define EOF (-1)

#ifdef __cplusplus
extern "C" {
#endif

#define true 1
#define false 0
#define bool _Bool

int printf(const char* __restrict, ...);
int putchar(int);
void terminal_clear();
void terminal_init();
char getchar();
unsigned int getkey();
void terminal_backup();
void terminal_setcolor(uint8_t color);

#ifndef _VGA_COLOR
#define _VGA_COLOR
enum vga_color {
	VGA_COLOR_BLACK = 0,
	VGA_COLOR_BLUE = 1,
	VGA_COLOR_GREEN = 2,
	VGA_COLOR_CYAN = 3,
	VGA_COLOR_RED = 4,
	VGA_COLOR_MAGENTA = 5,
	VGA_COLOR_BROWN = 6,
	VGA_COLOR_LIGHT_GREY = 7,
	VGA_COLOR_DARK_GREY = 8,
	VGA_COLOR_LIGHT_BLUE = 9,
	VGA_COLOR_LIGHT_GREEN = 10,
	VGA_COLOR_LIGHT_CYAN = 11,
	VGA_COLOR_LIGHT_RED = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN = 14,
	VGA_COLOR_WHITE = 15,
};

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
	return fg | bg << 4;
}
#endif

typedef struct {
	uint64_t location;
	uint64_t size;
	uint8_t mountNumber;
	uint32_t clusterNumber; //For FAT based systems
	uint64_t dir_entry;
	_Bool writelock;
	_Bool valid;
	_Bool directory;
} FILE, *PFILE;

FILE fopen(const char* filename, const char* mode);
uint8_t fread(FILE *file, char *buf, uint64_t start, uint64_t len);
uint8_t fwrite(FILE *file, char *buf, uint64_t start, uint64_t len);
FILE fcreate(char *filename);
uint8_t fdelete(char *filename);
FILE readdir(FILE *d, char* buf, uint32_t n);

char *getenv(char *name);

void exit(uint32_t code);

#ifdef __cplusplus
}
#endif

#endif
