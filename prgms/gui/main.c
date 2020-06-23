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

uint8_t min = 0;
uint8_t selected = 0;
uint8_t count = 0;
FILE currdir;
char name[16][31];
char buf[31];
char cd[4096];
char program[4096];
uint32_t g_argc;

uint8_t disks[8];
uint8_t numdisks;
uint8_t diskselected = 0;

bool diskselector() {
	for (uint8_t i = 0; i < numdisks; i++) {
		if (i==diskselected) {
			terminal_setcolor(0x9F);
		} else {
			terminal_setcolor(0xF0);
		}
		terminal_goto(36,9+(i));
		if (disks[i]!=0xFF)
			printf(" %c:/ ",65+disks[i]);
		else {
			printf("     ");
		}
	}
	terminal_setcolor(0xF0);
	terminal_goto(24,20);
	while(1){
		uint8_t g = getkey();
		if (g==0x50||g==0xD0) {
			if (diskselected<numdisks-1)
				diskselected++;
			return false;
		}
		if (g==0xC8||g==0x48) {
			if (diskselected>0)
				diskselected--;
			return false;
		}
		if (g==0x1C||g==0x9C) {
			getchar();
			cd[0] = 65+disks[diskselected];
			cd[1] = ':';
			cd[2] = '/';
			cd[3] = 0;
			return true;
		}
		if (g==0x01||g==0x81) {
			return true;
		}
	}
}

bool programselector() {
	for (uint8_t i = min; i < min+12; i++) {
		if (i==selected) {
			terminal_setcolor(0x9F);
		} else {
			terminal_setcolor(0xF0);
		}
		terminal_goto(24,8+(i-min));
		if (i<count) {
			printf(" %s ",name[i]);
		}
		else {
			printf("                                ");
		}
	}
	terminal_setcolor(0xF0);
	terminal_goto(24,20);
	if (count-min>12)
		printf("               \031\031\031                ");
	else {
		printf("        End of directory       ");
	}
	while(1){
		uint8_t g = getkey();
		if (g==0x50||g==0xD0) {
			if (selected<count-1)
				selected++;
			if (selected>=min+12)
				min++;
			return false;
		}
		if (g==0xC8||g==0x48) {
			if (selected>0)
				selected--;
			if (selected<min)
				min--;
			return false;
		}
		if (g==0x1C||g==0x9C) {
			getchar();
			if (strcmp(name[selected],"../                           ")) {
				*strrchr(cd,'/')=0;
				*(strrchr(cd,'/')+1)=0;
				return true;
			} else if (strcmp(name[selected],"./                            ")) {
				return true;
			} else if (strchr(name[selected],'/')) {
				*(strrchr(name[selected],'/')+1)=0;
				strcpy(cd+strlen(cd),name[selected]);
				return true;
			} else {
				char *prgm = name[selected];
				*strchr(prgm,' ')=0;
				if (strcmp(prgm+strlen(prgm)-4,".PRG")||strcmp(prgm+strlen(prgm)-4,".SYS")) {
					char *p_argv[1];
					p_argv[0]=NULL;
					char *p_envp[3];
					p_envp[0]="CD";
					p_envp[1]=cd;
					p_envp[2]=NULL;
					strcpy(program,cd);
					strcpy(program+strlen(program),prgm);
					terminal_setcolor(0x0F);
					terminal_clear();
					uint32_t pid = exec_args(program,p_argv,p_envp);
					waitpid(pid);
					printf("Press a key to return to file manager...");
					getkey();
					while(!getkey())
						;
					return true;
				} else if (strcmp(prgm+strlen(prgm)-4,".TXT")||strcmp(prgm+strlen(prgm)-4,".RTF")) {
					char *p_argv[2];
					p_argv[0]=prgm;
					p_argv[1]=NULL;
					char *p_envp[3];
					p_envp[0]="CD";
					p_envp[1]=cd;
					p_envp[2]=NULL;
					strcpy(program,cd);
					strcpy(program+strlen(program),prgm);
					terminal_setcolor(0x0F);
					terminal_clear();
					uint32_t pid = exec_args("A:/BIN/CAT.PRG",p_argv,p_envp);
					waitpid(pid);
					printf("Press a key to return to file manager...");
					getkey();
					while(!getkey())
						yield();
					return true;
				} else {
					prgm[strlen(prgm)]=' ';
				}
			}
		}
		if (g==0x01||g==0x81) {
			if (g_argc>0) {
				terminal_setcolor(0x0F);
				terminal_clear();
				exit(0);
			}
		}
		if (g==0x3B||g==0xBB) {
			drawrect(15,5,50,16,0x0F);
			drawrect(17,6,46,14,0xF0);
			terminal_goto(24,7);
			printf("Use the cursor to select a disk:");
			numdisks = 0;
			char *testdisk = "#:/";
			for (uint8_t i = 0; i < 8; i++) {
				testdisk[0] = 65+i;
				FILE f = fopen(testdisk,"r");
				if (f.valid) {
					disks[numdisks] = i;
					numdisks++;
				}
			}
			while(1) {
				if (diskselector())
					break;
			}
			return true;
		}
	}
}



void gui() {
	terminal_setcolor(0x40);
	terminal_clear();
	terminal_setcolor(0x70);
	terminal_writestring("                             TritiumOS File Browser                             ");
	terminal_goto(0,24);
	terminal_writestring(" Esc = exit | F1 = Change disk                                                 ");
	terminal_putentryat(' ',0x70,79,24);
	drawrect(20,2,40,21,0x0F);
	terminal_setcolor(0xF0);
	drawrect(22,3,36,3,0xF0);
	terminal_goto(23,4);
	printf("Use the cursor to select a program");
	drawrect(22,7,36,15,0xF0);
	while(1) {
		if (programselector())
			return;
	}
}

_Noreturn void main(uint32_t argc, char **argv) {
	g_argc = argc;
	//terminal_init();
	char *env_cd = getenv("CD");
	if (!env_cd) {
		strcpy(cd,"A:/");
	} else {
		strcpy(cd,env_cd);
	}
	getkey();
	for (;;) {
		count = 0;
		selected = 0;
		min = 0;
		currdir = fopen(cd,"r");
		memset(name,0,sizeof(char)*16*31);
		FILE r = currdir;
		for (uint8_t i = 0; i < 16 && r.valid; i++) {
			memset(buf,0,31);
			r = readdir(&currdir,buf,i);
			if (r.valid&&buf[0]) {
				memset(name[count],' ',30);
				memcpy(name[count],buf,strlen(buf));
				for (uint8_t j = 0; j < 30; j++) {
					if (!name[count][j])
						name[count][j]=' ';
				}
				count++;
			}
		}
		gui();
	}
}