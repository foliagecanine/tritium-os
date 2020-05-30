#include <kernel/stdio.h>
#include <fs/file.h>

char *currentDirectory;
char commandPart[256];

bool check_command(char* command) {
	bool cmdAck = false;
	bool usingNewline = true;
	bool breakcode = false;
	if (strcmp(command, "help")||strcmp(command, "?")) {
		printf("HELP:\nhelp, ? - Show this menu\nver, version - Show the OS version\ncls, clear - Clear the terminal\ntime - display the current time and date\ntzone [tz] - set current time zone (ex. -7,+0,+7)\nlsmnt - list all mounts\nmount [disk] mount physical disk (0-3)\ndir,ls - list files in current directory\ncat [file] - display the text contents of a file\nreboot - reboot the computer\nexit - exit terminal and reboot");
		cmdAck=true;
	}
	
	if (strcmp(command, "ver")||strcmp(command, "version")) {
		printf("TritiumOS Kernel v0.1");
		cmdAck=true;
	}
	
	if (strcmp(command, "cls")||strcmp(command,"clear")) {
		terminal_clear();
		usingNewline = false;
		cmdAck=true;
	}
	
	if (strcmp(command,"lsmnt")) {
		for (uint8_t i = 0; i < 8; i++) {
				if (getDiskMount(i).mountEnabled) printf("mnt%d (%c): %s%d, %s\n", i, (const char []){'A','B','C','D','E','F','G','H'}[i], "SATA", getDiskMount(i).drive, getDiskMount(i).type);
		}
		usingNewline = false;
		cmdAck=true;
	}

	if (strcmp(command, "color")) {
		terminal_setcolor(0x70);
		terminal_refresh();
		usingNewline = false;
		cmdAck=true;
	}
	
	memset(commandPart,0,strlen(command)+1);
	strcut(command,commandPart,0,5);
	if (strcmp(commandPart,"mount")) {
		if (strlen(command)>6) {
			memset(commandPart,0,strlen(command)+1);
			strcut(command,commandPart,6,strlen(command));
			char * possibleValues[] = { "0", "1", "2", "3", "4", "5", "6", "7", "8" };
			for (uint8_t i = 0; i < 9; i++) {
				if (strcmp(commandPart,possibleValues[i])) {
					char *statuses[] = {"SUCCESS","INCORRECT_FS_TYPE","DRIVE_IN_USE","UNKNOWN_ERROR"};
					printf("Mount finished with status: %s\n",statuses[mountDrive(i)]);
					break;
				} else if (i==8) {
					printf("Unknown drive number: %s\n",commandPart);
				}
			}
		} else {
			printf("You must specify the disk to mount (0-3).\n");
		}
		cmdAck=true;
	}
	
	memset(commandPart,0,strlen(command)+1);
	strcut(command,commandPart,0,7);
	if (strcmp(commandPart,"unmount")||strcmp(commandPart,"umount ")) {
		if (strlen(command)>6) {
			memset(commandPart,0,strlen(command)+1);
			strcut(command,commandPart,7,strlen(command));
			char * possibleValues[] = { "0", "1", "2", "3", "4", "5", "6", "7", "8" };
			for (uint8_t i = 0; i < 9; i++) {
				if (strcmp(commandPart,possibleValues[i])) {
					char *statuses[] = {"SUCCESS","INCORRECT_FS_TYPE","DRIVE_IN_USE","UNKNOWN_ERROR"};
					printf("Unmount finished with status: %s\n",statuses[unmountDrive(i)]);
					break;
				} else if (i==8) {
					printf("Unknown drive number: %s\n",commandPart);
				}
			}
		} else {
			printf("You must specify the disk to mount (0-3).\n");
		}
		cmdAck=true;
	}
	
	if (strcmp(command,"ls")||strcmp(command,"dir")) {
		FILE cd =  fopen(currentDirectory, "r");
		if (!cd.valid) {
			if (!getDiskMount(cd.mountNumber).mountEnabled)
				printf("Error: disk 0 not mounted.");
			else
				printf("Could not open folder %s for reading.",currentDirectory);
		} else {
			FAT12_print_folder((uint32_t)cd.location,(uint32_t)cd.size,getDiskMount(tolower(currentDirectory[0])-'a').drive);
		}
		cmdAck = true;
	}
	
	memset(commandPart,0,strlen(command)+1);
	strcut(command,commandPart,0,2);
	if (strcmp(commandPart,"cd")) {
		memset(commandPart,0,strlen(command)+1);
		strcut(command,commandPart,3,strlen(command));
		
		char filename[256];
		memset(filename,0,256);
		if (commandPart[1]==':') {
			strcpy(filename,commandPart);
		} else {
			strcpy(filename,currentDirectory);
			strcpy(filename+strlen(currentDirectory),commandPart);
		}
		
		if (filename[strlen(commandPart)-1]!='/') {
			memset(currentDirectory,0,4096);
			strcpy(currentDirectory,filename);
			currentDirectory[strlen(filename)]='/';
		} else {
			memset(currentDirectory,0,4096);
			strcpy(currentDirectory,filename);
		}
		cmdAck = true;
	}
	
	memset(commandPart,0,strlen(command)+1);
	strcut(command,commandPart,0,4);
	if (strcmp(commandPart,"cat ")) {
		memset(commandPart,0,strlen(command)+1);
		strcut(command,commandPart,4,strlen(command));
		char filename[strlen(currentDirectory)+strlen(commandPart)+1];
		if (commandPart[1]==':') {
			strcpy(filename,commandPart);
		} else {
			strcpy(filename,currentDirectory);
			strcpy(filename+strlen(currentDirectory),commandPart);
		}
		FILE f = fopen(filename,"r");
		if (!f.valid) {
			printf("Error: file \"%s\" not found.\n",filename);
		} else if (f.directory) {
			printf("Error: \"%s\" is a directory.\n",filename);
		} else {
			char data[f.size+1];
			memset(data,0,f.size+1);
			fread(&f,data,0,f.size);
			printf(data);
		}
		usingNewline = false;
		cmdAck = true;
	} else if (strcmp(commandPart,"cat")) {
		printf("Please specify filename.\n");
		cmdAck = true;
		usingNewline = false;
	}
	
	memset(commandPart,0,strlen(command)+1);
	strcut(command,commandPart,0,3);
	if (strcmp(commandPart,"run")) {
		memset(commandPart,0,strlen(command)+1);
		strcut(command,commandPart,4,strlen(command));
		char filename[strlen(currentDirectory)+strlen(commandPart)+1];
		if (commandPart[1]==':') {
			strcpy(filename,commandPart);
		} else {
			strcpy(filename,currentDirectory);
			strcpy(filename+strlen(currentDirectory),commandPart);
		}
		FILE f = fopen(filename,"r");
		if (!f.valid) {
			printf("Error: file \"%s\" not found.\n",filename);
		} else if (f.directory) {
			printf("Error: \"%s\" is a directory.\n",filename);
		} else {
			void * read = map_page_to((void *)0x100000);
			memset(read,0,4096);
			fread(&f,read,0,4096);
			asm("jmp 0x100000");
		}
		usingNewline = false;
		cmdAck = true;
	}
	
	if (strcmp(command, "exit")) {
		breakcode = true;
		cmdAck=true;
	}

	if (!cmdAck) {
		printf("Unknown command:\n");
		printf("%s\n",command);
	}
	if (usingNewline) {
		printf("\n");
	}
	return breakcode;
}

void debug_console() {
	printf("Debug console active!\n");
	printf("\nWARNING: This console is HIGHLY UNSTABLE and runs in KERNEL MODE.\nThat means that it can easily crash or break things.\n\n");
	currentDirectory = (char *)alloc_page(1);
	memset(currentDirectory,0,4096);
	strcpy(currentDirectory,"A:/");
	
	char lastCommand[256];
	uint8_t lastNumChars = 0;
	memset(lastCommand,0,256);
	
	char thisCommand[256];
	uint8_t thisNumChars = 0;
	memset(thisCommand,0,256);

	
	while (true) {
		printf("%s>",currentDirectory);
		char command[256];
		memset(command,0,255);
		uint8_t numChars = 0;
		uint32_t k = 0;
		while(scancode_to_char(k)!='\n') {
			if (k==72) {
				memcpy(thisCommand,command,256);
				thisNumChars = numChars;
				for (uint8_t i = 0; i < numChars; i++) {
					terminal_backup();
					printf(" ");
					terminal_backup();
				}
				memcpy(command,lastCommand,256);
				numChars = lastNumChars;
				printf(command);
			}
				
			if (k==80) {
				for (uint8_t i = 0; i < numChars; i++) {
					terminal_backup();
					printf(" ");
					terminal_backup();
				}
				memcpy(command,thisCommand,256);
				numChars = thisNumChars;
				printf(command);
			}
			
			if (numChars<255&&scancode_to_char(k)&&k!=72&&k!=80) {
				printf("%c",scancode_to_char(k));
				command[numChars]=scancode_to_char(k);
				numChars++;
			}
			if (numChars>0&&k==14) {
				terminal_backup();
				printf(" ");
				terminal_backup();
				numChars--;
				command[numChars]=0;
			}
			k = getkey();
		}
		
		memcpy(lastCommand,command,256);
		lastNumChars = numChars;
		
		printf("\n");
		check_command(command);
	}
}
