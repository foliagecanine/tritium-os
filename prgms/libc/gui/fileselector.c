#include <stdio.h>
#include <string.h>
#include <tty.h>
#include <gui.h>

uint8_t __fselect_min = 0;
uint8_t __fselect_selected = 0;
uint8_t __fselect_count = 0;
char __fselect_name[16][31];
char *__fselect_cd;

uint8_t __fselect_disks[8];
uint8_t __fselect_numdisks;
uint8_t __fselect_diskselected = 0;

void __fselect_draw_disk() {
	for (uint8_t i = 0; i < __fselect_numdisks; i++) {
		if (i==__fselect_diskselected) {
			terminal_setcolor(0x9F);
		} else {
			terminal_setcolor(0xF0);
		}
		terminal_goto(36,9+(i));
		if (__fselect_disks[i]!=0xFF)
			printf(" %c:/ ",65+__fselect_disks[i]);
		else {
			printf("     ");
		}
	}
	terminal_setcolor(0xF0);
	terminal_goto(24,20);
}

char *__fselect_newfile() {
	drawrect(26,8,28,9,0x0F);
	drawrect(28,9,24,7,0xF0);
	terminal_setcolor(0x0F);
	terminal_goto(36,8);
	printf("New File");
	terminal_setcolor(0xF0);
	terminal_goto(35,11);
	printf("File name:");
	terminal_goto(33,13);
	terminal_setcolor(0x70);
	printf("              ");
	terminal_enablecursor(0,15);
	terminal_setcursor(34,13);
	int8_t cursor_x = 0;
	char newname[13];
	memset(newname,0,13);
	while(true) {
		uint8_t g = getkey();
		char c = getchar();
		if (g==1)
			return 0;
		else if (g==0x0E) {
			if (cursor_x > 0) {
				cursor_x--;
				newname[cursor_x] = 0;
				terminal_putentryat(' ',0x70,34+cursor_x,13);
				terminal_setcursor(34+cursor_x,13);
			}
		} else if (c) {
			if (c=='\n') {
				memcpy(&__fselect_cd[strlen(__fselect_cd)],newname,13);
				return __fselect_cd;
			} else if (cursor_x<12&&c!='\t'&&c!='\\'&&c!='/'&&c!=':'&&c!='*'&&c!='"'&&c!='<'&&c!='>'&&c!='|'&&c!='?') {
				terminal_putentryat(c,0x70,34+cursor_x,13);
				newname[cursor_x] = c;
				cursor_x++;
				terminal_setcursor(34+cursor_x,13);
			}
		}
	}
}

uint8_t __fselect_disk() {
	__fselect_draw_disk();
	while(1){
		uint8_t g = getkey();
		if (g==0x50||g==0xD0) {
			if (__fselect_diskselected<__fselect_numdisks-1)
				__fselect_diskselected++;
			__fselect_draw_disk();
		}
		if (g==0xC8||g==0x48) {
			if (__fselect_diskselected>0)
				__fselect_diskselected--;
			__fselect_draw_disk();
		}
		if (g==0x1C||g==0x9C) {
			getchar();
			return __fselect_disks[__fselect_diskselected];
		}
		if (g==0x01||g==0x81) {
			return 0xFF;
		}
	}
}

void __fselect_update() {
	__fselect_count = 0;
	__fselect_min = 0;
	__fselect_selected = 0;
	FILE currdir;
	char buf[31];
	currdir = fopen(__fselect_cd,"r");
	memset(__fselect_name,0,sizeof(char)*16*31);
	FILE r = currdir;
	for (uint8_t i = 0; i < 16 && r.valid; i++) {
		memset(buf,0,31);
		r = readdir(&currdir,buf,i);
		if (r.valid&&buf[0]) {
			memset(__fselect_name[__fselect_count],' ',30);
			memcpy(__fselect_name[__fselect_count],buf,strlen(buf));
			for (uint8_t j = 0; j < 30; j++) {
				if (!__fselect_name[__fselect_count][j])
					__fselect_name[__fselect_count][j]=' ';
			}
			__fselect_count++;
		}
	}
}

void __fselect_draw_select(bool newfile_button) {
	terminal_setcolor(0xE0);
	terminal_clear();
	terminal_setcolor(0x70);
	terminal_writestring("                             TritiumOS File Browser                             ");
	terminal_goto(0,24);
	if (!newfile_button)
		terminal_writestring(" Esc = exit | F1 = Change disk                                                 ");
	else
		terminal_writestring(" Esc = exit | F1 = Change disk | F3 = New File                                 ");
	terminal_putentryat(' ',0x70,79,24);
	drawrect(20,2,40,21,0x0F);
	terminal_setcolor(0xF0);
	drawrect(22,3,36,3,0xF0);
	terminal_goto(23,4);
	printf("Use the cursor to select a file...");
	drawrect(22,7,36,15,0xF0);
}

char *__fselect_select(bool newfile) {
	for (uint8_t i = __fselect_min; i < __fselect_min+12; i++) {
		if (i==__fselect_selected) {
			terminal_setcolor(0x9F);
		} else {
			terminal_setcolor(0xF0);
		}
		terminal_goto(24,8+(i-__fselect_min));
		if (i<__fselect_count) {
			printf(" %s ",__fselect_name[i]);
		}
		else {
			printf("                                ");
		}
	}
	terminal_setcolor(0xF0);
	terminal_goto(24,20);
	if (__fselect_count-__fselect_min>12)
		printf("               \031\031\031                ");
	else {
		printf("        End of directory       ");
	}
	while(1){
		uint8_t g = getkey();
		if (g==0x50) {
			if (__fselect_selected<__fselect_count-1)
				__fselect_selected++;
			if (__fselect_selected>=__fselect_min+12)
				__fselect_min++;
			return 0;
		}
		if (g==0x48) {
			if (__fselect_selected>0)
				__fselect_selected--;
			if (__fselect_selected<__fselect_min)
				__fselect_min--;
			return 0;
		}
		if (g==0x1C) {
			getchar();
			if (strcmp(__fselect_name[__fselect_selected],"../                           ")) {
				*strrchr(__fselect_cd,'/')=0;
				*(strrchr(__fselect_cd,'/')+1)=0;
			} else if (strcmp(__fselect_name[__fselect_selected],"./                            ")) {
			} else if (strchr(__fselect_name[__fselect_selected],'/')) {
				*(strrchr(__fselect_name[__fselect_selected],'/')+1)=0;
				strcpy(__fselect_cd+strlen(__fselect_cd),__fselect_name[__fselect_selected]);
			} else {
				*strchr(__fselect_name[__fselect_selected],' ') = 0;
				strcpy(__fselect_cd+strlen(__fselect_cd),__fselect_name[__fselect_selected]);
				return __fselect_cd;
			}
			__fselect_update();
			return 0;
		}
		if (g==0x01) {
			return (char *)1;
		}
		if (g==0x3B) {
			drawrect(15,5,50,16,0x0F);
			drawrect(17,6,46,14,0xF0);
			terminal_goto(24,7);
			printf("Use the cursor to select a disk:");
			__fselect_numdisks = 0;
			char *testdisk = "#:/";
			for (uint8_t i = 0; i < 8; i++) {
				testdisk[0] = 65+i;
				FILE f = fopen(testdisk,"r");
				if (f.valid) {
					__fselect_disks[__fselect_numdisks] = i;
					__fselect_numdisks++;
				}
			}
			char d = __fselect_disk();
			__fselect_cd[0] = 65+d;
			__fselect_cd[1] = ':';
			__fselect_cd[2] = '/';
			__fselect_cd[3] = 0;
			__fselect_update();
			__fselect_draw_select(newfile);
			return 0;
		}
		if (newfile&&g==0x3D) {
			char *r = __fselect_newfile();
			terminal_clearcursor();
			if (r) {
				fcreate(r);
			}
			return r;
		}
	}
}

char *fileselector(char *cd, bool newfile) {
	strcpy(cd,"A:/");
	__fselect_cd = cd;
	__fselect_update();
	__fselect_draw_select(newfile);
	while(1) {
		char *r = __fselect_select(newfile);
		if (r)
			return r;
	}
}