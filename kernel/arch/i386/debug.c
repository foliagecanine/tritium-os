#include <kernel/stdio.h>
#include <fs/file.h>
#include <kernel/mem.h>

char *currentDirectory;
char commandPart[256];

void graphicstest();

bool breakcode = false;

bool check_command(char* command) {
	bool cmdAck = false;
	bool usingNewline = true;
	
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
	
	if (strcmp(command, "gtest")) {
		graphicstest();
		for(;;);
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

	
	while (!breakcode) {
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










//Graphis tests down here

void VGA_write_register(uint32_t reg, uint8_t value) {
	uint16_t port = (reg>>16)&0xFFFF;
	uint8_t addr = reg&0xFF;
	//printf("Writing %d to %# index %#\n",(uint32_t)value,(uint64_t)port,(uint64_t)addr);
	outb(port,addr);
	outb(port+1,value);
}

uint8_t VGA_read_register(uint32_t reg) {
	uint16_t port = (reg>>16)&0xFFFF;
	uint8_t addr = reg&0xFF;
	outb(port,addr);
	return inb(port+1);
}

typedef struct {
	uint32_t SeqMemoryMode;
	uint32_t GraphicsMode;
	uint32_t MapMask;
	uint32_t EnableSetReset;
	uint32_t SetReset;
	uint32_t DataRotate;
	uint32_t Bitmask;
	uint32_t HorizontalTotal;
	uint32_t EndHorizontalDisplay;
	uint32_t StartHorizontalBlanking;
	uint32_t EndHorizontalBlanking;
	uint32_t StartHorizontalRetrace;
	uint32_t EndHorizontalRetrace;
	uint32_t VerticalTotal;
	uint32_t Overflow;
	uint32_t MaxScanLine;
	uint32_t StartVerticalRetrace;
	uint32_t EndVerticalRetrace;
	uint32_t EndVerticalDisplay;
	uint32_t StartVerticalBlanking;
	uint32_t EndVerticalBlanking;
	uint32_t ClockingMode;
	uint32_t CharacterSelect;
	uint32_t Misc;
	uint32_t PresetRowScan;
	uint32_t LogicalWidth;
	uint32_t UnderlineLocation;
	uint32_t ModeControl;
	uint32_t Sequencer;
} _VGAReg;

_VGAReg VGAReg;

void VGA_default_values() {
	//Sequencer
	VGAReg.Sequencer				= 0x03C40000;
	VGAReg.ClockingMode				= 0x03C40001;
	VGAReg.MapMask 					= 0x03C40002;
	VGAReg.CharacterSelect			= 0x03C40003;
	VGAReg.SeqMemoryMode			= 0x03C40004;
	
	//Graphics Controller
	VGAReg.SetReset 				= 0x03CE0000;
	VGAReg.EnableSetReset 			= 0x03CE0001;
	VGAReg.DataRotate 				= 0x03CE0003;
	VGAReg.GraphicsMode 			= 0x03CE0005;
	VGAReg.Misc						= 0x03CE0006;
	VGAReg.Bitmask 					= 0x03CE0008;
	
	//CRT Registers
	VGAReg.HorizontalTotal			= 0x03D40000;
	VGAReg.EndHorizontalDisplay		= 0x03D40001;
	VGAReg.StartHorizontalBlanking	= 0x03D40002;
	VGAReg.EndHorizontalBlanking	= 0x03D40003;
	VGAReg.StartHorizontalRetrace	= 0x03D40004;
	VGAReg.EndHorizontalRetrace		= 0x03D40005;
	
	VGAReg.VerticalTotal			= 0x03D40006;
	VGAReg.Overflow					= 0x03D40007;
	VGAReg.PresetRowScan			= 0x03D40008;
	VGAReg.MaxScanLine				= 0x03D40009;
	VGAReg.StartVerticalRetrace		= 0x03D40010;
	VGAReg.EndVerticalRetrace		= 0x03D40011;
	VGAReg.EndVerticalDisplay		= 0x03D40012;
	VGAReg.LogicalWidth				= 0x03D40013;
	VGAReg.UnderlineLocation		= 0x03D40014;
	VGAReg.StartVerticalBlanking	= 0x03D40015;
	VGAReg.EndVerticalBlanking		= 0x03D40016;
	VGAReg.ModeControl				= 0x03D40017;
}

uint32_t vga256colors[] = {
	
};

void color_palette_set(uint8_t id, uint8_t r, uint8_t g, uint8_t b) {
	outb(0x3C8,id);
	outb(0x3C9,r);
	outb(0x3C9,g);
	outb(0x3C9,b);
}

void graphicstest() {
	VGA_default_values();
	
	//Get access to 0xA0000-0xBFFFF (VGA memory)
	for (uint32_t i = 0xA0000; i < 0xBFFFF; i+=4096) {
		identity_map((void *)i);
	}
	
	//Unlock CRTC registers (disable bit 8)
	VGA_write_register(VGAReg.EndVerticalRetrace,VGA_read_register(VGAReg.EndVerticalRetrace)&0x7F);
	//Stop sequencer
	VGA_write_register(VGAReg.Sequencer,1);

	//
	//Load all registers (values from https://wiki.osdev.org/VGA_Hardware)
	//

	//Misc Out Register
	outb(0x3C2,0x63);

	//Sequencer
	VGA_write_register(VGAReg.ClockingMode,0x01);
	VGA_write_register(VGAReg.MapMask,0x0F);
	VGA_write_register(VGAReg.CharacterSelect,0x00);
	VGA_write_register(VGAReg.SeqMemoryMode,0x0E);
	
	//CRTC Horizontal
	VGA_write_register(VGAReg.HorizontalTotal,0x5F);
	VGA_write_register(VGAReg.EndHorizontalDisplay,0x4F);
	VGA_write_register(VGAReg.StartHorizontalBlanking,0x50);
	VGA_write_register(VGAReg.EndHorizontalBlanking,0x82);
	VGA_write_register(VGAReg.StartHorizontalRetrace,0x54);
	VGA_write_register(VGAReg.EndHorizontalRetrace,0x80);
	VGA_write_register(VGAReg.LogicalWidth,0x28);
	
	//CRTC Vertical
	VGA_write_register(VGAReg.VerticalTotal,0xBF);
	VGA_write_register(VGAReg.Overflow,0x1F);
	VGA_write_register(VGAReg.MaxScanLine,0x41);
	VGA_write_register(VGAReg.StartVerticalRetrace,0x9C);
	VGA_write_register(VGAReg.EndVerticalRetrace,0x8E);
	VGA_write_register(VGAReg.EndVerticalDisplay,0x8F);
	VGA_write_register(VGAReg.StartVerticalBlanking,0x96);
	VGA_write_register(VGAReg.PresetRowScan,0x00);
	VGA_write_register(VGAReg.EndVerticalBlanking,0xB9);
	VGA_write_register(VGAReg.UnderlineLocation,0x40);
	VGA_write_register(VGAReg.ModeControl,0xA3);
	
	//Grapics Controller
	VGA_write_register(VGAReg.GraphicsMode,0x40);
	VGA_write_register(VGAReg.Misc,0x05);
	
	//Attributes Controller
	//Mode control
	inb(0x3DA);
	outb(0x3C0,0x10);
	outb(0x3C0,0x41);
	//Overscan
	inb(0x3DA);
	outb(0x3C0,0x11);
	outb(0x3C0,0x00);
	//ColorPlaneEnable
	inb(0x3DA);
	outb(0x3C0,0x12);
	outb(0x3C0,0x0F);
	//Horizontal Panning
	inb(0x3DA);
	outb(0x3C0,0x13);
	outb(0x3C0,0x00);
	//ColorSelect
	inb(0x3DA);
	outb(0x3C0,0x14);
	outb(0x3C0,0x00);
	
	for (uint16_t i = 0; i < 256; i++) {
		color_palette_set(i,i+23,i+83,i);
	}
	
	//Lock CRTC registers (enable bit 8)
	VGA_write_register(VGAReg.EndVerticalRetrace,VGA_read_register(VGAReg.EndVerticalRetrace)|0x80);
	//Enable display
	inb(0x3DA);
	outb(0x3C0,0x20);
	//Start sequencer
	VGA_write_register(VGAReg.Sequencer,3);

	memset((void *)0xA0000,0,0x20000);

	while(1) {
		//for (uint8_t i = 0; i != 255; i++) {
			for (uint32_t j = 0; j < 0xFA00; j++) {
				memset((void *)0xA0000+j,j%256,1);
				if (!(j%20))
					sleep(1);
			}
		//}
	}
}










