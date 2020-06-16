#include <stdio.h>
#include <string.h>

__asm__("jmp main");

static inline void syscall(unsigned int syscall_num) {
	asm volatile("mov %0,%%eax;int $0x80"::"r"(syscall_num));
}

void writestring(char *string) {
	asm volatile("mov %0,%%ebx"::"r"(string));
	syscall(0);
}

uint32_t exec(char *name) {
	uint32_t retval;
	asm volatile("mov %0,%%ebx"::"r"(name));
	syscall(1);
	asm volatile("mov %%eax,%0":"=m"(retval):);
	return retval;
}

void yield() {
	syscall(6);
}

uint32_t waitpid(uint32_t pid) {
	uint32_t retval;
	asm volatile("mov %0,%%ebx"::"r"(pid));
	syscall(10); //waitpid
	syscall(11); //get_retval
	asm volatile("mov %%eax,%0":"=m"(retval):);
	return retval;
}

uint32_t getpid() {
	uint32_t retval;
	syscall(7);
	asm volatile("mov %%eax,%0":"=m"(retval):);
	return retval;
}

char cmd[256] = "";
char temp[256] = "";
char cd[256] = "A:/";
uint8_t index = 0;
uint8_t child_pid = 0;
bool failed = false;

void commandline() {
	printf("%s>",cd);
	for(;;) {
		char c = 0;
		while (!c) {
			c = getchar();
			if (!c) {
				if (getkey()==14&&index) {
					terminal_backup();
					putchar(' ');
					terminal_backup();
					index--;
					cmd[index]=0;
				}
			}
			yield();
		}
		if (c=='\n') {
			printf("\n");
			break;
		}
		if (index!=249) {
			putchar(c);
			cmd[index] = c;
			cmd[index+1] = 0;
			index++;
		}
	}
	memset(temp,0,256);
	memcpy(temp,cmd,3);
	temp[4]=0;
	if (strcmp(temp,"cd ")) {
		memcpy(temp,cmd+3,253);
		temp[255]=0;
		if (temp[1]==':') {
			memcpy(cd,temp,256);
		} else {
			if (cd[strlen(cd)-1]!='/') {
				cd[strlen(cd)+1]=0;
				cd[strlen(cd)]='/';
			}
			memcpy(cd+strlen(cd),temp,256-strlen(cd));
		}
		if (strlen(cd)!=3&&cd[strlen(cd)-1]=='/') {
			cd[strlen(cd)-1]=0;
		}
	} else if (strcmp(cmd,"cls")||strcmp(cmd,"clear")) {
		terminal_clear();
	} else if (strcmp(cmd,"exit")) {
		if (getpid()!=1) {
			syscall(2);
		}
	} else {
		/*cmd[strlen(cmd)]='.';
		cmd[strlen(cmd)]='P';
		cmd[strlen(cmd)]='R';
		cmd[strlen(cmd)]='G';
		cmd[strlen(cmd)]=0;
		memset(temp,0,256);
		strcpy(temp,"A:/bin/");
		memcpy(temp+strlen(temp),cmd,256-strlen(temp));
		printf("Running %s...\n",temp);
		child_pid = exec(temp);
		if (!child_pid) {
			memset(temp,0,256);
			strcpy(temp,"A:/prgms/");
			for (uint8_t i = 9; i < 255; i++) {
				temp[i]=cmd[i-9];
			}
			printf("Running %s...\n",temp);
			child_pid = exec(temp);
			if (!child_pid) {
				strcpy(temp,cd);
				uint8_t off=0;
				if (cd[strlen(cd)-1]!='/')
					off=1;
				memcpy(temp+strlen(cd)+off,cmd,256-(strlen(cd)+2));
				temp[strlen(cd)-1+off]='/';
				temp[256]=0;
				printf("Running %s...\n",temp);
				child_pid = exec(temp);
				if (!child_pid) {
					printf("Error reading file: %s!\n",cmd);
				}
			}
		}*/
		
		memset(temp,0,256);
		strcpy(temp,"A:/bin/");
		for (uint8_t i = 7; i != 0; i++) {
			temp[i]=cmd[i-7];
		}
		temp[strlen(temp)]='.';
		temp[strlen(temp)]='P';
		temp[strlen(temp)]='R';
		temp[strlen(temp)]='G';
		temp[strlen(temp)]=0;
		
		printf("Running %s...\n",temp);
		child_pid = exec(temp);
		
		if (!child_pid) {
			memset(temp,0,256);
			strcpy(temp,"A:/prgms/");
			for (uint8_t i = 9; i != 0; i++) {
				temp[i]=cmd[i-9];
			}
			temp[strlen(temp)]='.';
			temp[strlen(temp)]='P';
			temp[strlen(temp)]='R';
			temp[strlen(temp)]='G';
			temp[strlen(temp)]=0;
			
			printf("Running %s...\n",temp);
			child_pid = exec(temp);
			
			if (!child_pid) {
				memset(temp,0,256);
				strcpy(temp,cd);
				if (temp[strlen(temp)-1]!='/')
					temp[strlen(temp)]='/';
				uint8_t c = strlen(temp);
				for (uint8_t i = c; i != 0; i++) {
					temp[i]=cmd[i-c];
				}
				temp[strlen(temp)]='.';
				temp[strlen(temp)]='P';
				temp[strlen(temp)]='R';
				temp[strlen(temp)]='G';
				temp[strlen(temp)]=0;

				printf("Running %s...\n",temp);
				child_pid = exec(temp);
				
				if (!child_pid) {
					memset(temp,0,256);
					strcpy(temp,"A:/bin/");
					for (uint8_t i = 7; i != 0; i++) {
						temp[i]=cmd[i-7];
					}
					temp[strlen(temp)]='.';
					temp[strlen(temp)]='S';
					temp[strlen(temp)]='Y';
					temp[strlen(temp)]='S';
					temp[strlen(temp)]=0;

					printf("Running %s...\n",temp);
					child_pid = exec(temp);
					
					if (!child_pid) {
						memset(temp,0,256);
						strcpy(temp,"A:/prgms/");
						for (uint8_t i = 9; i != 0; i++) {
							temp[i]=cmd[i-9];
						}
						temp[strlen(temp)]='.';
						temp[strlen(temp)]='S';
						temp[strlen(temp)]='Y';
						temp[strlen(temp)]='S';
						temp[strlen(temp)]=0;

						printf("Running %s...\n",temp);
						child_pid = exec(temp);
						
						if (!child_pid) {
							memset(temp,0,256);
							strcpy(temp,cd);
							if (temp[strlen(temp)-1]!='/')
								temp[strlen(temp)]='/';
							uint8_t d = strlen(temp);
							for (uint8_t i = d; i != 0; i++) {
								temp[i]=cmd[i-d];
							}
							temp[strlen(temp)]='.';
							temp[strlen(temp)]='S';
							temp[strlen(temp)]='Y';
							temp[strlen(temp)]='S';
							temp[strlen(temp)]=0;
							
							printf("Running %s...\n",temp);
							child_pid = exec(temp);
							if (!child_pid) {
								printf("No file found.\n");
								failed = true;
							}
						}
					}
				}
			}
		}
	}
	
	if (!failed) {
		waitpid(child_pid);
	}
	
	index = 0;
	memset(cmd,0,256);
	
}

_Noreturn void main() {
	writestring("Hello from SHELL.SYS!\n");
	terminal_init();
	printf("ElectronShell online.\n");
	for(;;) {
		commandline();
	}
}