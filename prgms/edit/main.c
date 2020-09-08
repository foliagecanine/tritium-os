#include <stdio.h>
#include <string.h>
#include <tty.h>
#include <gui.h>

static inline void syscall(unsigned int syscall_num) {
	asm volatile("mov %0,%%eax;int $0x80"::"r"(syscall_num));
}

char file[1024*32]; //Reserve 32KiB for file. That is max size.
char lines[2048][1024]; // Files can be up to 2048 lines of 1023 chars. Takes 2 MiB of program space.
char fname[4096];

char *ctext_cursor = file;
uint32_t text_cursor_x = 0;
uint32_t text_cursor_y = 1;
int32_t skip_lines = -1;
uint32_t hscroll = 0;
uint16_t numlines = 0;

void update_cursor() {
	terminal_setcursor(text_cursor_x-hscroll,text_cursor_y);
	terminal_goto(30,24);
	terminal_setcolor(0x70);
	printf("Line %d, Col %d     ",text_cursor_y,text_cursor_x);
}

void parse(char *f) {
	uint16_t currentline = 0;
	uint16_t currentchar = 0;
	memset(lines,0,2048*1024);
	for (uint16_t i = 0; i < 1024*32; i++) {
		if (!f[i])
			break;
		if (currentchar==1023||f[i]=='\n') {
			currentline++;
			currentchar = 0xFFFF;
		} else {
			lines[currentline][currentchar] = f[i];
		}
		currentchar++;
	}
	numlines = currentline;
}

void load(char *filename) {
	if (filename) {
		memset(file,0,4096);
		FILE f = fopen(fname,"r");
		if (f.valid)
			fread(&f,file,0,f.size<1024*32?f.size:1024*32);
		parse(file);
	}
}

void save(char *filename) {
	if ((uint32_t)filename<2)
		return;
	char *ptr = &lines[0][0];
	for (uint16_t i = 0; i < 2048; i++) {
		if (lines[i][0])
			lines[i][strlen(lines[i])]='\n';
		memcpy(ptr,lines[i],strlen(lines[i]));
		if (i<2047)
			ptr = &lines[0][0]+strlen(lines[0]);
	}
	memset(&lines[0][0]+strlen(lines[0]),0,(2048*1024)-strlen(lines[0]));
	FILE f = fopen(filename,"w");
	fwrite(&f,&lines[0][0],0,strlen(lines[0]));
	load(filename);
}

void redraw(bool all) {
	if (all) {
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
		terminal_writestring("elp\263                  TritiumOS Text Editor                              ");
		terminal_goto(0,24);
		terminal_writestring(" Hold Alt to access menus                                                      ");
		terminal_putentryat(' ',0x70,79,24);
		terminal_setcolor(0xF0);
		for (uint8_t i = 1; i < 23; i++) {
			terminal_goto(0,i);
			if (lines[i+skip_lines][0]) {
				for (uint8_t j = 0; j < 80; j++) {
					if (!lines[i+skip_lines][j+hscroll])
						break;
					putchar(lines[i+skip_lines][j+hscroll]);
				}
				putchar('\n');
			}
		}
	} else {
		terminal_setcolor(0xF0);
		terminal_goto(0,text_cursor_y);
		for (uint8_t i = 0; i < 80; i++)
			putchar(' ');
		terminal_goto(0,text_cursor_y);
		for (uint8_t j = 0; j < 80; j++) {
			if (!lines[text_cursor_y+skip_lines][j+hscroll])
				break;
			putchar(lines[text_cursor_y+skip_lines][j+hscroll]);
		}
	}
	update_cursor();
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
	char c = getchar();
	while(c != '\n')
		c = getchar();
}

bool key_left_arrow () {
	uint8_t req_redraw = 0;
	if (text_cursor_x>0) {
		text_cursor_x--;
		if (text_cursor_x<hscroll+1&&hscroll!=0) {
			hscroll = text_cursor_x-1;
			req_redraw = 2;
		}
	} else if (text_cursor_y>1||skip_lines>-1) {
		if (text_cursor_y==1) {
			if (skip_lines!=-1) {
				skip_lines--;
				req_redraw = 2;
			}
		} else {
			text_cursor_y--;
		}
		size_t l_next_line = strlen(lines[skip_lines+text_cursor_y]);
		text_cursor_x = l_next_line;
		hscroll = text_cursor_x > 79 ? text_cursor_x - 79 : 0;
		if (hscroll!=0)
			req_redraw = 2;
	}
	return req_redraw;
}

uint8_t key_right_arrow () {
	uint8_t req_redraw = 0;
	if (text_cursor_x<strlen(lines[skip_lines+text_cursor_y])) {
		text_cursor_x++;
		if (text_cursor_x > hscroll+79) {
			hscroll++;
			req_redraw = 2;
		}
	} else {
		if (skip_lines+text_cursor_y+1<numlines) {
			if (text_cursor_y==23) {
					skip_lines++;
			} else {
				text_cursor_y++;
			}
			text_cursor_x = 0;
			hscroll = 0;
			req_redraw = 2;
		}
	}
	return req_redraw;
}

uint8_t key_up_arrow () {
	uint8_t req_redraw = false;
	if (text_cursor_y==1) {
		if (skip_lines!=-1) {
			skip_lines--;
			req_redraw = 2;
		}
	} else {
		text_cursor_y--;
	}
	size_t l_next_line = strlen(lines[skip_lines+text_cursor_y]);
	if (text_cursor_x > l_next_line) {
		text_cursor_x = l_next_line;
		if (text_cursor_x>79) {
			hscroll = text_cursor_x-79;
			req_redraw = 2;
		}
	}
	return req_redraw;
}

uint8_t key_down_arrow () {
	uint8_t req_redraw = false;
	if (skip_lines+text_cursor_y+1<numlines) {
		if (text_cursor_y==23) {
				skip_lines++;
				req_redraw = 2;
		} else {
			text_cursor_y++;
		}
		size_t l_next_line = strlen(lines[skip_lines+text_cursor_y]);
		if (text_cursor_x > l_next_line) {
			text_cursor_x = l_next_line;
			if (text_cursor_x>79) {
				hscroll = text_cursor_x-79;
				req_redraw = 2;
			}
		}
	}
	return req_redraw;
}

void gui() {
	redraw(true);
	while(1) {
		int g = getkey();
		char c = getchar();
		
		if (g==0x48) { // Up arrow
			if (key_up_arrow())
				redraw(true);
			else 
				update_cursor();
		}
		
		if (g==0x50) { // Down arrow
			if (key_down_arrow())
				redraw(true);
			else 
				update_cursor();
		}
		
		if (g==0x4D) { // Right arrow
			if (key_right_arrow())
				redraw(true);
			else 
				update_cursor();
		}
		
		if (g==0x4B) { // Left Arrow
			if (key_left_arrow())
				redraw(true);
			else 
				update_cursor();
		}
		
		if (g==0x0E) { // Backspace
			if (text_cursor_x>0) {
				key_left_arrow();
				memcpy(&lines[skip_lines+text_cursor_y][text_cursor_x],&lines[skip_lines+text_cursor_y][text_cursor_x+1],1023-text_cursor_x);
				redraw(hscroll==text_cursor_x-1);
			} else {
				uint32_t old_tcx = text_cursor_x;
				uint32_t old_y = text_cursor_y+skip_lines;
				key_left_arrow();
				if (old_tcx != text_cursor_x || old_y != text_cursor_y+skip_lines) {
					memcpy(&lines[skip_lines+text_cursor_y][text_cursor_x],&lines[skip_lines+text_cursor_y+1][0],1023-text_cursor_x);
					for (uint32_t i = skip_lines+text_cursor_y+1; i < 2046; i++) {
						memcpy(&lines[i],&lines[i+1],1024);
					}
					lines[2047][0] = 0;
					redraw(true);
				}
				numlines--;
			}
		}
		
		if (g==0x53) { // Delete
			if (text_cursor_x<strlen(lines[skip_lines+text_cursor_y])) {
				memcpy(&lines[skip_lines+text_cursor_y][text_cursor_x],&lines[skip_lines+text_cursor_y][text_cursor_x+1],1023-text_cursor_x);
				} else {
				memcpy(&lines[skip_lines+text_cursor_y][text_cursor_x],&lines[skip_lines+text_cursor_y+1][0],1023-text_cursor_x);
				for (uint32_t i = skip_lines+text_cursor_y+1; i < 2046; i++) {
					memcpy(&lines[i],&lines[i+1],1024);
				}
				lines[2047][0] = 0;
			}
			redraw(true);
		}
		
		if (c&&g!=0x4D&&g!=0x4B&&g!=0x48&&g!=0x50&&g!=0x53&&g!=0x0E) {
			if (c=='\n') {
				for (uint16_t i = 2047; i > skip_lines+text_cursor_y; i--) {
					memcpy(&lines[i+1][0],&lines[i][0],1024);
				}
				memset(&lines[skip_lines+text_cursor_y+1][0],0,1024-strlen(lines[skip_lines+text_cursor_y]));
				memcpy(&lines[skip_lines+text_cursor_y+1],&lines[skip_lines+text_cursor_y][text_cursor_x],strlen(lines[skip_lines+text_cursor_y])-text_cursor_x);
				memset(&lines[skip_lines+text_cursor_y][text_cursor_x],0,1024-strlen(lines[skip_lines+text_cursor_y]));
				numlines++;
				key_right_arrow();
				redraw(true);
			} else {
				if (text_cursor_x<1023) {
					text_cursor_x++;
					memmove(&lines[skip_lines+text_cursor_y][text_cursor_x],&lines[skip_lines+text_cursor_y][text_cursor_x-1],1024-text_cursor_x);
					lines[skip_lines+text_cursor_y][text_cursor_x-1] = c;
					if (text_cursor_x>79) {
						hscroll++;
						redraw(true);
					} else {
						redraw(false);
					}
				}
			}
			terminal_setcolor(0x70);
			terminal_goto(70,24);
			printf("%#",(uint64_t)g);
			update_cursor();
		}
		
		if (g==0x38) { // Alt key
			terminal_clearcursor();
			while (g!=0xB8&&g!=0x01) { // Release Alt key
				g = getkey();
				char c = getchar();
				if (c=='f') {
					redraw(true);
					drawrect(0,1,10,4,0x70);
					terminal_goto(0,1);
					terminal_setcolor(0x78);
					terminal_putchar('N');
					terminal_setcolor(0x70);
					printf("ew");
					terminal_goto(0,2);
					terminal_setcolor(0x78);
					terminal_putchar('O');
					terminal_setcolor(0x70);
					printf("pen");
					terminal_goto(0,3);
					terminal_setcolor(0x78);
					terminal_putchar('S');
					terminal_setcolor(0x70);
					printf("ave as");
					terminal_goto(0,4);
					terminal_setcolor(0x70);
					terminal_putchar('E');
					terminal_setcolor(0x78);
					terminal_putchar('x');
					terminal_setcolor(0x70);
					printf("it");
				}
				if (c=='h') {
					redraw(true);
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
				if (c=='s') {
					save(fileselector(fname,true));
					break;
				}
				if (c=='o') {
					memset(fname,0,4096);
					char *fs = fileselector(fname,true);
					if ((uint32_t)fs>1) {
						FILE f = fopen(fs,"r");
						if (f.valid) {
							memset(file,0,1024*32);
							fread(&f,file,0,f.size<1024*32?f.size:1024*32);
						}
						parse(file);
						text_cursor_y = 1;
						text_cursor_x = 0;
						terminal_setcursor(0,1);
					}
					break;
				}
				if (c=='n') {
					memset(lines,0,2048*1024);
					memset(fname,0,4096);
					text_cursor_x = 0;
					text_cursor_y = 1;
					break;
				}
			}
			terminal_enablecursor(0,15);
			redraw(true);
		}
		
		
	}
}

void main(uint32_t argc, char **argv) {
	getkey();
	memset(file,0,1024*16);
	memset(lines,0,2048*1024);
	memset(fname,0,4096);
	terminal_enablecursor(0,15);
	terminal_setcursor(0,1);
	char *env_cd = getenv("CD");
	if (argc>1) {
		memset(fname,0,4096);
		if (argv[1][1]==':')
			strcpy(fname,argv[1]);
		else {
			memcpy(fname,env_cd,strlen(env_cd));
			memcpy(fname+strlen(env_cd),argv[1],strlen(argv[1]));
		}
		load(fname);
	}
	while(1) {
		gui();
	}
}
