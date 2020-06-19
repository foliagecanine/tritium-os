#include <stdio.h>
#include <string.h>
__asm__("push %ecx; push %eax; call main");

static inline void syscall(unsigned int syscall_num) {
	asm volatile("mov %0,%%eax;int $0x80"::"r"(syscall_num));
}

void writestring(char *string) {
	asm volatile("mov %0,%%ebx"::"r"(string));
	syscall(0);
}

uint32_t exec(char *name) {
	uint32_t retval;
	asm volatile("mov %0,%%ebx; mov $0,%%ecx"::"r"(name));
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
	asm volatile("mov %0,%%ebx"::"r"(pid));
	syscall(10); //waitpid
	syscall(6); //yield
	syscall(6); //yield
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
char args[256] = "";
char temp[256] = "";
char cd[256] = "A:/";
char *envp[4096];
uint8_t index = 0;
uint8_t child_pid = 0;
bool failed = false;

void commandline() {
	failed = false;
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
			
			memcpy(cd+strlen(cd),temp,256-strlen(cd));
		}
		if (strlen(cd)!=3&&cd[strlen(cd)-1]=='/') {
			cd[strlen(cd)-1]=0;
		}
		if (cd[strlen(cd)-1]!='/') {
			cd[strlen(cd)+1]=0;
			cd[strlen(cd)]='/';
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
		
		if (cmd[0]=='&') {
			memcpy(cmd,cmd+1,255); //Slide characters over by 1
			failed = true; //Effectively disable waitpid
		}
		
		//Environment variables
		memset(envp,0,4096);
		envp[0] = "CD";
		envp[1] = cd;
		envp[2] = NULL;

		//Arguments
		char *argstart = strchr(cmd,' ');
		char *argv[256+128];
		uint8_t argc = 0;
		memset(argv,0,sizeof(char *)*128);
		argv[0]=NULL;
		if (argstart) {
			memset(args,0,256);
			strcpy(args,argstart+1);
			memset(argstart,0,256-strlen(cmd));
			uint8_t max = strlen(args);
			uint8_t argvptr = 0;
			argc = 1;
			argv[0]=args;
			
			//Replace all instances of space with zero to denote end of string
			for (char *i = args; i<max+args; i++) {
				if (*i==' ') {
					*i=0;
					argv[++argvptr] = i+1;
					argc++;
				}
			}
		}
		
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
		child_pid = exec_args(temp,argv,envp);
		
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
			child_pid = exec_args(temp,argv,envp);
			
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
				child_pid = exec_args(temp,argv,envp);
				
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
					child_pid = exec_args(temp,argv,envp);
					
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
						child_pid = exec_args(temp,argv,envp);
						
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
							child_pid = exec_args(temp,argv,envp);
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

uint8_t buf[513];

_Noreturn void main(uint32_t argc, char **argv) {
	writestring("Hello from SHELL.SYS!\n");
	terminal_init();
	printf("ElectronShell online.\n");
	if (!argc||!argv) { //This only happens when the kernel launches us. User-launched programs always have at least one argument
		FILE f = fopen("A:/MOTD.TXT","r");
		if (f.valid) {
			if (!f.directory) {
				for (uint32_t i = 0; i < f.size; i+=512) {
					fread(&f,buf,i,512);
					printf("%s",buf);
				}
			} else {
				printf("Error: cannot read MOTD.\n");
			}
		} else {
			printf("Error: cannot read MOTD.\n");
		}
	}
	for(;;) {
		commandline();
	}
}