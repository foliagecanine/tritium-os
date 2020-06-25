#include <stdio.h>
#include <string.h>
#include <tty.h>
extern char **envp;
extern uint32_t envc;
asm ("push %ecx;\
		push %eax;\
		mov %ebx,(envc);\
		mov %edx,(envp);\
		call main");

static inline void syscall(unsigned int syscall_num) {
	asm volatile("mov %0,%%eax;int $0x80"::"r"(syscall_num));
}

void writestring(char *string) {
	asm volatile("mov %0,%%ebx"::"r"(string));
	syscall(0);
}

uint32_t exec(char *name) {
	uint32_t retval;
	asm volatile("mov %0,%%ebx; mov $0,%%ecx; mov $0,%%edx"::"r"(name));
	syscall(1);
	asm volatile("mov %%eax,%0":"=m"(retval):);
	return retval;
}

uint32_t exec_args(char *name, char **arguments, char **environment) {
	uint32_t retval;
	asm volatile("pusha; mov %0,%%ebx; mov %1,%%ecx; mov %2,%%edx"::"m"(name),"m"(arguments),"m"(environment));
	syscall(1);
	asm volatile("mov %%eax,%0; popa":"=m"(retval):);
	return retval;
}

void yield() {
	syscall(6);
}

uint32_t waitpid(uint32_t pid) {
	uint32_t retval = 1;
	asm volatile("pusha; mov %0,%%ebx"::"r"(pid));
	syscall(10); //waitpid
	syscall(11); //get_retval
	asm volatile("mov %%eax,%0; popa":"=m"(retval):);
	return retval;
}

uint32_t getpid() {
	uint32_t retval;
	syscall(7);
	asm volatile("mov %%eax,%0":"=m"(retval):);
	return retval;
}

void drawrect(size_t x, size_t y, size_t w, size_t h, uint8_t color) {
	for (uint8_t _y = y; _y < y + h; _y++) {
	terminal_goto(x,_y);
		for (uint8_t _x = x; _x < x + w; _x++) {
			terminal_putentryat(' ',color,_x,_y);
		}
	}
}

char file[1024*16]; //Reserve 16KiB for file. That is max size.
char decode[1024*16]; //Reserve 16KiB for decoding. That is max size.
char *lines[4096]; //Max 4096 lines.

char *ctext_cursor = file;
uint32_t text_cursor_x = 0;
uint32_t text_cursor_y = 1;
int32_t skip_lines = -1;

void update_cursor() {
	terminal_setcursor(text_cursor_x%80,text_cursor_y+text_cursor_x/80);
}

void redraw() {
	drawrect(0,1,80,23,0xF0);
	terminal_setcolor(0x70);
	terminal_goto(0,0);
	terminal_writestring("\263");
	terminal_setcolor(0x78);
	terminal_writestring("F");
	terminal_setcolor(0x70);
	terminal_writestring("ile\263");
	terminal_setcolor(0x78);
	terminal_writestring("H");
	terminal_setcolor(0x70);
	terminal_writestring("elp\263                  TritiumOS File Viewer                              ");
	terminal_goto(0,24);
	terminal_writestring("                                                                               ");
	terminal_putentryat(' ',0x70,79,24);
	terminal_setcolor(0xF0);
	for (uint32_t i = 1; i < 23; i++) {
		terminal_goto(0,i);
		if (lines[i+skip_lines])
			printf(lines[i+skip_lines]);
	}
}

void message_ok() {
	drawrect(14,7,51,11,0x0F);
	drawrect(16,8,47,9,0xF0);
	terminal_goto(36,9);
	terminal_setcolor(0xF0);
	printf("EDIT.PRG");
	terminal_goto(32,10);
	printf("by foliagecanine");
	terminal_goto(19,12);
	terminal_setcolor(0xF1);
	printf("http://github.com/foliagecanine/tritium-os");
	terminal_goto(36,15);
	terminal_setcolor(0x9F);
	printf("   OK   ");
	unsigned int g = 0;
	while(g!=0x9C)
		g = getkey();
}

void gui() {
	redraw();
	while(1) {
		int g = getkey();
		if (g==0x48) {
			if (text_cursor_y==1) {
				if (skip_lines!=-1) {
					skip_lines--;
					redraw();
				}
			} else {
				text_cursor_y--;
			}
			size_t l_next_line = strlen(lines[skip_lines+text_cursor_y]);
			if (text_cursor_x > l_next_line)
				text_cursor_x = l_next_line;
			update_cursor();
		}
		if (g==0x50) {
			if (lines[skip_lines+text_cursor_y+1]) {
				if (text_cursor_y==23) {
						skip_lines++;
						redraw();
				} else {
					text_cursor_y++;
				}
				size_t l_next_line = strlen(lines[skip_lines+text_cursor_y]);
				if (text_cursor_x > l_next_line)
					text_cursor_x = l_next_line;
				update_cursor();
			}
		}
		if (g==0x4D) {
			if (text_cursor_x<strlen(lines[skip_lines+text_cursor_y]))
				text_cursor_x++;
			update_cursor();
		}
		if (g==0x4B) {
			if (text_cursor_x>0)
				text_cursor_x--;
			update_cursor();
		}
		if (g==0x38) {
			terminal_clearcursor();
			while (g!=0xB8) {
				g = getkey();
				char c = getchar();
				if (c=='f') {
					redraw();
					drawrect(0,1,10,1,0x70);
					terminal_goto(0,1);
					terminal_setcolor(0x70);
					terminal_putchar('E');
					terminal_setcolor(0x78);
					terminal_putchar('x');
					terminal_setcolor(0x70);
					printf("it");
				}
				if (c=='h') {
					redraw();
					drawrect(5,1,15,1,0x70);
					terminal_goto(5,1);
					terminal_setcolor(0x78);
					terminal_putchar('A');
					terminal_setcolor(0x70);
					printf("bout");
				}
				if (c=='a') {
					message_ok();
					break;
				}
				if (c=='x') {
					terminal_setcolor(0x07);
					terminal_clear();
					terminal_clearcursor();
					exit(0);
				}
			}
			terminal_enablecursor(0,15);
			redraw();
		}
	}
}

void parse(char *f, char *o, char *l[4096]) {
	memcpy(o,f,1024*16);
	memset(l,0,4096);
	o[(1024*16)-1] = 0;
	uint16_t cl = 1;
	l[0] = o;
	for (uint16_t j = 0; j < 1024*16;) {
		for (uint8_t i = 0; i < 80; i++, j++) {
			if (o[j]=='\n')
				break;
		}
		if (o[j]==0)
			break;
		o[j]=0;
		l[cl]=o+j+1;
		cl++;
	}
}

char fname[4096];

_Noreturn void main(uint32_t argc, char **argv) {
	getkey();
	memset(file,0,1024*16);
	memset(fname,0,4096);
	terminal_enablecursor(0,15);
	terminal_setcursor(0,1);
	if (argc>1) {
		char *cd = getenv("CD");
		memset(fname,0,4096);
		FILE f;
		if (argv[1][1]==':')
			strcpy(fname,argv[1]);
		else {
			memcpy(fname,cd,strlen(cd));
			memcpy(fname+strlen(cd),argv[1],strlen(argv[1]));
		}
		f = fopen(fname,"r");
		if (f.valid)
			fread(&f,file,0,f.size<1024*16?f.size:1024*16);
	}
	parse(file,decode,lines);
	while(1) {
		gui();
	}
}