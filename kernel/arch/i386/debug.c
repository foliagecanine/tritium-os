#include <kernel/debug.h>

#define bool _Bool

char *currentDirectory;

bool check_command(char* command) {
	bool cmdAck = false;
	bool usingNewline = true;
	bool breakcode = false;
	if (strcmp(command, "help")||strcmp(command, "?")) {
		printf("HELP:\nhelp, ? - Show this menu\nver, version - Show the OS version\ncls, clear - Clear the terminal\ntime - display the current time and date\ntzone [tz] - set current time zone (ex. -7,+0,+7)\nmount [disk] mount physical disk (0-3)\ndir,ls - list files in current directory\ncat [file] - display the text contents of a file\nreboot - reboot the computer\nexit - exit terminal and reboot");
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
	
	if (strcmp(command,"reboot")) {
		reboot();
	}

	if (strcmp(command, "color")) {
		terminal_setcolor(0x70);
		terminal_refresh();
		usingNewline = false;
		cmdAck=true;
	}

	if (strcmp(command, "time")) {
		read_rtc();
		printf("%d",(uint32_t)current_hour_time());
		printf(":");
		printf("%d",(uint32_t)current_minute_time());
		printf(":");
		printf("%d",(uint32_t)current_second_time());
		printf("\n");
		printf("%d",(uint32_t)current_month_time());
		printf("/");
		printf("%d",(uint32_t)current_day_time());
		printf("/");
		printf("%d",(uint32_t)current_year_time());
		usingNewline = true;
		cmdAck = true;
	}
	
	char commandPart[strlen(command)+1];
	memset(commandPart,0,strlen(command)+1);
	strcut(command,commandPart,0,5);
	if (strcmp(commandPart,"tzone")) {
		if (strlen(command)>6) {
			memset(commandPart,0,strlen(command)+1);
			strcut(command,commandPart,6,strlen(command));
			const char* possibleValues[] = { "-12","-11","-10","-9","-8","-7","-6","-5","-4","-3","-2","-1","+0","+1","+2","+3","+4","+5","+6","+7","+8","+9","+10","+11","+12","+13","+14","-9:30","-3:30","-2:30","+3:30","+4:30","+5:30","+5:45","+6:30","+8:45","+9:30","+10:30","+12:45","+13:45"
															};
			for (int i=-12;i<15;i++) {
				if (strcmp(possibleValues[i+12],commandPart)){
					set_time_zone(i);
					printf("Successfully set time zone: UTC");
					printf(commandPart);
					break;
				} else if (i==14) {
					printf("Error: Time zone did not match a value in possible time zone list. Did you format it correctly? (e.g. -7 for UTC-7 and +7 for UTC+7)\n");
					printf("Entered time zone: UTC%s",commandPart);
				}
			}
		} else {
			printf("Error: Must specify time zone (e.g. -7 for UTC-7 and +7 for UTC+7)");
		}
		usingNewline = true;
		cmdAck = true;
	}
	
	memset(commandPart,0,strlen(command)+1);
	strcut(command,commandPart,0,5);
	if (strcmp(commandPart,"mount")) {
		if (strlen(command)>6) {
			memset(commandPart,0,strlen(command)+1);
			strcut(command,commandPart,6,strlen(command));
			char * possibleValues[] = { "0", "1", "2", "3" };
			for (uint8_t i = 0; i < 4; i++) {
				if (strcmp(commandPart,possibleValues[i])) {
					char *statuses[] = {"SUCCESS","INCORRECT_FS_TYPE","DRIVE_IN_USE","UNKNOWN_ERROR"};
					printf("Mount finished with status: %s\n",statuses[mountDrive(i)]);
					break;
				} else if (i==3) {
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
			char * possibleValues[] = { "0", "1", "2", "3" };
			for (uint8_t i = 0; i < 4; i++) {
				if (strcmp(commandPart,possibleValues[i])) {
					char *statuses[] = {"SUCCESS","INCORRECT_FS_TYPE","DRIVE_IN_USE","UNKNOWN_ERROR"};
					printf("Unmount finished with status: %s\n",statuses[unmountDrive(i)]);
					break;
				} else if (i==3) {
					printf("Unknown drive number: %s\n",commandPart);
				}
			}
		} else {
			printf("You must specify the disk to mount (0-3).\n");
		}
		cmdAck=true;
	}
	
	if (strcmp(command,"ls")||strcmp(command,"dir")) {
		char stripcd[strlen(currentDirectory)-1];
		memcpy(stripcd,currentDirectory,strlen(currentDirectory)-1);
		FILE cd =  fopen(currentDirectory, "r");
		if (!cd.valid) {
			if (!getDiskMount(cd.mountNumber).mountEnabled)
				printf("Error: disk 0 not mounted.");
			else
				printf("Could not open folder %s for reading.",currentDirectory);
		} else {
			FAT12_print_folder((uint32_t)cd.location,(uint32_t)cd.size,tolower(currentDirectory[0])-'a');
		}
		cmdAck = true;
	}
	
	memset(commandPart,0,strlen(command)+1);
	strcut(command,commandPart,0,2);
	if (strcmp(commandPart,"cd")) {
		free(currentDirectory);
		memset(commandPart,0,strlen(command)+1);
		strcut(command,commandPart,3,strlen(command));
		
		char filename[strlen(currentDirectory)+strlen(commandPart)+1];
		memset(filename,0,strlen(currentDirectory)+strlen(commandPart)+1);
		if (commandPart[1]==':') {
			strcpy(filename,commandPart);
		} else {
			strcpy(filename,currentDirectory);
			strcpy(filename+strlen(currentDirectory),commandPart);
		}
		
		if (filename[strlen(commandPart)-1]!='/') {
			currentDirectory = malloc(sizeof(filename)+1);
			strcpy(currentDirectory,filename);
			currentDirectory[strlen(filename)]='/';
		} else {
			currentDirectory = malloc(sizeof(filename));
			strcpy(currentDirectory,filename);
		}
		cmdAck = true;
	}
	
	memset(commandPart,0,strlen(command)+1);
	strcut(command,commandPart,0,3);
	if (strcmp(commandPart,"cat")) {
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
		free(filename);
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
	currentDirectory = (char *)malloc(sizeof("A:/"));
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
		if (check_command(command))
			reboot();
	}
}
