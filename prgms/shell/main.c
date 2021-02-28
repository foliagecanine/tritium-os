#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline void syscall(unsigned int syscall_num) {
	asm volatile("mov %0,%%eax;int $0x80"::"r"(syscall_num));
}

char cmd[256] = "";
char args[256] = "";
char temp[256] = "";
char cd[256] = "A:/";
char *_envp[4096];
uint8_t idx = 0;
uint8_t child_pid = 0;
bool failed = false;

void change_dir() {
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
}

void commandline() {
	failed = false;
	printf("%s>",cd);
	for(;;) {
		char c = 0;
		while (!c) {
			c = getchar();
			if (!c) {
				if (getkey()==14&&idx) {
					terminal_backup();
					putchar(' ');
					terminal_backup();
					idx--;
					cmd[idx]=0;
				}
			}
			yield();
		}
		if (c=='\n') {
			printf("\n");
			break;
		}
		if (idx!=249) {
			putchar(c);
			cmd[idx] = c;
			cmd[idx+1] = 0;
			idx++;
		}
	}
	memset(temp,0,256);
	memcpy(temp,cmd,3);
	temp[4]=0;
	if (!strcmp(temp,"cd ")) {
		change_dir();
	} else if (!strcmp(cmd,"cls")||!strcmp(cmd,"clear")) {
		terminal_clear();
	} else if (!strcmp(cmd,"exit")) {
		if (getpid()!=1) {
			exit(0);
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
		memset(_envp,0,4096);
		_envp[0] = "CD";
		_envp[1] = cd;
		_envp[2] = NULL;

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
		
		//printf("Running %s...\n",temp);
		child_pid = exec_args(temp,argv,_envp);
		
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
			
			//printf("Running %s...\n",temp);
			child_pid = exec_args(temp,argv,_envp);
			
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

				//printf("Running %s...\n",temp);
				child_pid = exec_args(temp,argv,_envp);
				
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

					//printf("Running %s...\n",temp);
					child_pid = exec_args(temp,argv,_envp);
					
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

						//printf("Running %s...\n",temp);
						child_pid = exec_args(temp,argv,_envp);
						
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
							
							//printf("Running %s...\n",temp);
							child_pid = exec_args(temp,argv,_envp);
							if (!child_pid) {
								printf("No file found.\n");
								failed = true;
							}
						}
					}
				}
			}
		}
		if (!failed) {
			waitpid(child_pid);
		}
	}
	
	idx = 0;
	memset(cmd,0,256);
}

uint8_t buf[513];

void main(uint32_t argc, char **argv) {
	writestring("Hello from SHELL.SYS!\n");
	terminal_init();
	printf("ElectronShell online.\n");
	/* if (!argc||!argv) { //This only happens when the kernel launches us. User-launched programs always have at least one argument
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
	} */
	for(;;) {
		commandline();
	}
}
