#include <kernel/stdio.h>
#include <fs/file.h>
#include <kernel/mem.h>
#include <kernel/multiboot.h>

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
		printf("TritiumOS Kernel v0.1\n");
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
	
	if (strcmp(command, "fsinfo")) {
		print_fat16_values(0);
		cmdAck=true;
	}
	
	if (strcmp(command, "fdelete")) {
		printf("Return: %d\n",fdelete("A:/TESTFILE.TXT"));
		cmdAck=true;
	}
	
	if (strcmp(command, "fcreate")) {
		printf("Return: %d\n",fcreate("A:/TESTFILE.TXT").valid);
		cmdAck=true;
	}
	
	if (strcmp(command, "fwrite")) {
		printf("Deleting. Return value: %d\n",fdelete("A:/TESTFILE.TXT"));
		FILE f = fcreate("A:/TESTFILE.TXT");
		//FILE f = fopen("A:/TESTFILE.TXT","w");
		if (!f.valid) {
			printf("Create failed.\n");
		}
		char text[] = "Hello world.\nABCDEFGHIJKLMNOPQRSTUVWXYZ\n";
		uint32_t len = strlen(text)+1;
		printf("Writing %d bytes...\n",len);
		uint8_t r = fwrite(&f,text,0,(uint64_t)len);
		printf("Return value: %d\n",r);
		cmdAck = true;
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
	
	if (strcmp(command, "shutdown")) {
		extern void bios32();
		identity_map((void *)0x7000);
		identity_map((void *)0x8000);
		kprint("[KMSG] Attempting shutdown using BIOS.");
		asm("\
		mov $0x1553,%ax;\
		call bios32;\
		");
		kerror("BIOS shutdown failed.");
		kprint("It is safe to turn off your computer.");
		asm("\
		1:cli;\
		hlt;\
		jmp 1\
		");
		for(;;);
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
			
			if (k<128&&numChars<255&&scancode_to_char(k)&&k!=72&&k!=80) {
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
	free_page(currentDirectory,1);
}










//Graphis tests down here

/*void VGA_write_register(uint32_t reg, uint8_t value) {
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
	uint32_t Plane;
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
	VGAReg.Plane					= 0x03CE0004;
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

void switch_plane(uint8_t plane) {
	VGA_write_register(VGAReg.Plane,plane);
	VGA_write_register(VGAReg.MapMask,1<<plane);
}

void _300x240x256() {
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
}*/

/*void _640x480x16() {
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
	outb(0x3C2,0xE3);

	//Sequencer
	VGA_write_register(VGAReg.ClockingMode,0x01);
	//VGA_write_register(VGAReg.MapMask,0x0F);
	VGA_write_register(VGAReg.CharacterSelect,0x00);
	VGA_write_register(VGAReg.SeqMemoryMode,0x02);
	
	//CRTC Horizontal
	VGA_write_register(VGAReg.HorizontalTotal,0x5F);
	VGA_write_register(VGAReg.EndHorizontalDisplay,0x4F);
	VGA_write_register(VGAReg.StartHorizontalBlanking,0x50);
	VGA_write_register(VGAReg.EndHorizontalBlanking,0x82);
	VGA_write_register(VGAReg.StartHorizontalRetrace,0x54);
	VGA_write_register(VGAReg.EndHorizontalRetrace,0x80);
	VGA_write_register(VGAReg.LogicalWidth,0x28);
	
	//CRTC Vertical
	VGA_write_register(VGAReg.VerticalTotal,0x0B);
	VGA_write_register(VGAReg.Overflow,0x3E);
	VGA_write_register(VGAReg.MaxScanLine,0x40);
	VGA_write_register(VGAReg.StartVerticalRetrace,0xEA);
	VGA_write_register(VGAReg.EndVerticalRetrace,0x8C);
	VGA_write_register(VGAReg.EndVerticalDisplay,0xDF);
	VGA_write_register(VGAReg.StartVerticalBlanking,0xE7);
	VGA_write_register(VGAReg.PresetRowScan,0x00);
	VGA_write_register(VGAReg.EndVerticalBlanking,0x04);
	VGA_write_register(VGAReg.UnderlineLocation,0x00);
	VGA_write_register(VGAReg.ModeControl,0xE3);
	
	//Grapics Controller
	VGA_write_register(VGAReg.GraphicsMode,0x00);
	VGA_write_register(VGAReg.Misc,0x05);
	
	//Attributes Controller
	//Mode control
	inb(0x3DA);
	outb(0x3C0,0x10);
	outb(0x3C0,0x01);
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
	
	for (uint16_t i = 0; i < 16; i++) {
		//color_palette_set(i,i,i,i);
	}
	
	//Lock CRTC registers (enable bit 8)
	VGA_write_register(VGAReg.EndVerticalRetrace,VGA_read_register(VGAReg.EndVerticalRetrace)|0x80);
	//Enable display
	inb(0x3DA);
	outb(0x3C0,0x20);
	//Start sequencer
	VGA_write_register(VGAReg.Sequencer,3);

	switch_plane(0);
	memset((void *)0xA0000,0,0x20000);
	switch_plane(1);
	memset((void *)0xA0000,0,0x20000);
	switch_plane(2);
	memset((void *)0xA0000,0,0x20000);
	switch_plane(3);
	memset((void *)0xA0000,0,0x20000);
}

uint8_t doublebuffer[640*480/2];*/

/* void p16pixel(uint32_t x, uint32_t y, uint16_t c) {
	uint32_t dataloc = (640*y)+x;
	uint8_t shift = 1<<(7-(dataloc%8));
	switch_plane(0);
	uint8_t *pixel = (uint8_t *)(0xA0000+(dataloc/8));
	*pixel ^= (-((c&1))^*pixel)&shift;
	switch_plane(1);
	*pixel ^= (-((c&2)>>1)^*pixel)&shift;
	switch_plane(2);
	*pixel ^= (-((c&4)>>2)^*pixel)&shift;
	switch_plane(3);
	*pixel ^= (-((c&8)>>3)^*pixel)&shift;
} */

/*void p16pixel(uint32_t x, uint32_t y, uint16_t c) {
	uint32_t dataloc = (640*y)+x;
	uint8_t shift = 1<<(7-(dataloc%8));
	uint8_t *pixel = (uint8_t *)(doublebuffer+(dataloc/8));
	*pixel ^= (-((c&1))^*pixel)&shift;
	pixel+=640*480/8;
	*pixel ^= (-((c&2)>>1)^*pixel)&shift;
	pixel+=640*480/8;
	*pixel ^= (-((c&4)>>2)^*pixel)&shift;
	pixel+=640*480/8;
	*pixel ^= (-((c&8)>>3)^*pixel)&shift;
}*/

uint32_t _size;
uint32_t _dst;
uint32_t _src;

void fastmemcpy32(uint32_t *dst, uint32_t *src, size_t size) {
	_size=size/4;
	_dst=(uint32_t)dst;
	_src=(uint32_t)src;
	asm("\
	mov _dst,%%edi;\
	mov _src,%%esi;\
	mov _size,%%ecx;\
	cld;\
	rep movsd;\
	":::"edi","esi","ecx");
}

/* void double_buffer() {
	switch_plane(0);
	fastmemcpy32((uint32_t *)0xA0000,(uint32_t *)doublebuffer,640*480/8);
	switch_plane(1);
	fastmemcpy32((uint32_t *)0xA0000,(uint32_t *)doublebuffer+(640*480/32),640*480/8);
	switch_plane(2);
	fastmemcpy32((uint32_t *)0xA0000,(uint32_t *)doublebuffer+((640*480/32)*2),640*480/8);
	switch_plane(3);
	fastmemcpy32((uint32_t *)0xA0000,(uint32_t *)doublebuffer+((640*480/32)*3),640*480/8);
	
	//memcpy(0xA0000,doublebuffer+(640*480/8),640*480/8);
	//memcpy(0xA0000,doublebuffer+((640*480/8)*2),640*480/8);
	//memcpy(0xA0000,doublebuffer+((640*480/8)*3),640*480/8);
} */

uint64_t font_table[] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //first 16 are just extra codes (nonprintable)
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //next 16 are too
  0x0000000000000000, //' ' char
  0x1010101010001000, //'!'
  0x1414000000000000, //See ascii reference for more (this one is value 34)
  0x00143E14143E1400, //These are raw binary encoded
  0x103C503814781000,
  0x7152740817254700, // as shown for this one (37='%') see below
  0x1C24182542463900,
  0x0808000000000000,
  0x0408101010080400, // 0 1 1 1 0 0 0 1 = 0x71
  0x1008040404081000, // 0 1 0 1 0 0 1 0 = 0x52
                      // 0 1 1 1 0 1 0 0 = 0x74
                      // 0 0 0 0 1 0 0 0 = 0x08
                      // 0 0 0 1 0 1 1 1 = 0x17
                      // 0 0 1 0 0 1 0 1 = 0x25
                      // 0 1 0 0 0 1 1 1 = 0x47
                      // 0 0 0 0 0 0 0 0 = 0x00

                      //When printed it should look like this:
  0x0000140814000000, //   # # #       #
  0x0008083E08080000, //   #   #     #
  0x0000000000081000, //   # # #   #
  0x0000003E00000000, //         #
  0x0000000000000800, //       #   # # #
  0x0102040810204000, //     #     #   #
  0x3844445444443800, //   #       # # #
  0x0818080808081C00, //
  0x3C42040810207E00, //The leftmost and bottommost sides are all 0's for spacing
  0x3C42021C02423C00, //Max chars on a 320x200 mode can be 40 horizontal and 25 vertical
  0x0C14247C04040400, //If you can figure out a mode bigger than 320x200 then good for you
  0x7E40407C02423C00, //Font made with scratch program at https://scratch.mit.edu/projects/298710977/
  0x3844407844443800,
  0x7E02040810204000,
  0x3844443844443800,
  0x3844443C04443800,
  0x0000200000200000,
  0x0000200000202000,
  0x030C3040300C0300,
  0x00003E00003E0000,
  0x6018060106186000,
  0x3844040810001000,
  0x3E41474947403E00,
  0x102844447C444400,
  0x7844447844447800,
  0x3844404040443800,
  0x7048444444487000,
  0x7C40407840407C00,
  0x7C40407840404000,
  0x3C4240404E423C00,
  0x4444447C44444400,
  0x7C10101010107C00,
  0x7C04040404443800,
  0x4448506050484400,
  0x4040404040407C00,
  0x4163554941414100,
  0x4161514945434100,
  0x3C42424242423C00,
  0x7844447840404000,
  0x3C42424242443A00,
  0x7844447844444400,
  0x3844403804443800,
  0x7F08080808080800,
  0x4444444444443800,
  0x4444444444281000,
  0x4141414149552200,
  0x4122140814224100,
  0x4444281010101000,
  0x7F02040810207F00,
  0x1C10101010101C00,
  0x4020100804020100,
  0x1C04040404041C00,
  0x0814220000000000,
  0x0000000000007F00,
  0x1010080000000000,
  0x0000700838483400,
  0x4040407048487000,
  0x0000003040403000,
  0x0808083848483800,
  0x0000304878403800,
  0x3048407040404000,
  0x0000384838087000,
  0x4040407048484800,
  0x0010001010101000,
  0x0008000808483000,
  0x4040485060504800,
  0x6020202020202000,
  0x000000446C545400,
  0x0000004070484800,
  0x0000003048483000,
  0x0000704870404000,
  0x0000384838080C00,
  0x0000007048404000,
  0x0000384078087000,
  0x0020702020201000,
  0x0000004848483000,
  0x0000004444281000,
  0x0000004444542800,
  0x0000442810284400,
  0x0000484838087000,
  0x0000007810207800,
  0x0C10102010100C00,
  0x0808080808080800,
  0x1804040204041800,
  0x324C000000000000
};

extern multiboot_info_t *mbi;
uint16_t framebuffer_width;
uint16_t framebuffer_height;
uint32_t framebuffer_pitch;
uint8_t framebuffer_bpp;
uint32_t *framebuffer_addr;
void *doublebuffer;

typedef struct {
	uint8_t b;
	uint8_t g;
	uint8_t r;
} __attribute__((packed)) color24;

typedef struct {
	uint8_t b;
	uint8_t g;
	uint8_t r;
	uint8_t a;
} __attribute__((packed)) color32;

void setpixel24(uint32_t x, uint32_t y, color24 c) {
	((color24 *)doublebuffer)[(framebuffer_width*y)+x]=c;
}

void setpixel32(uint32_t x, uint32_t y, color32 c) {
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
	((color32 *)doublebuffer)[(framebuffer_width*y)+x]=c;
}

void syncvideo() {
	fastmemcpy32(framebuffer_addr,doublebuffer,(framebuffer_height*framebuffer_pitch));
}

/*void draw_char(uint32_t x,uint32_t y,char c,color24 back_color, color24 front_color) {
	uint8_t *charval = (uint8_t *)&font_table[(uint8_t)c];
	for (uint8_t _y = 0; _y < 8; _y++) {
		for (uint8_t _x = 0; _x < 8; _x++) {
			setpixel24(x+_x,y+_y,(charval[7-(_y)]>>(7-(_x)))&1?front_color:back_color);
		}
	}
}*/

void draw_char_a(uint32_t x,uint32_t y,char c,void *front_color) {
	uint8_t *charval = (uint8_t *)&font_table[(uint8_t)c];
	for (uint8_t _y = 0; _y < 8; _y++) {
		for (uint8_t _x = 0; _x < 8; _x++) {
			if ((charval[7-(_y)]>>(7-(_x)))&1) {
				if (framebuffer_bpp==24)
					setpixel24(x+_x,y+_y,*(color24 *)front_color);
				else
					setpixel32(x+_x,y+_y,*(color32 *)front_color);
			}
		}
	}
}

void print_string(uint32_t x, uint32_t y, char *s, color24 front_color) {
	for (size_t i = 0; i < strlen(s); i++) {
		draw_char_a(((i*8)+x)%framebuffer_width,y+((((i*8)+x)/framebuffer_width)*8),s[i],&front_color);
	}
}

uint16_t resolutions[] = {
	1920, 1200,
	1920, 1080,
	1680, 1250,
	1600, 1200,
	1440, 1080,
	1440, 900,
	1366, 768,
	1280, 1024,
	1280, 960,
	1280, 854,
	1280, 768,
	1280, 720,
	1152, 864,
	1024, 768,
	800 , 600,
	720 , 480,
	640 , 480,
	640 , 350
};

uint8_t bpp[] = {
	32,
	24,
	16,
	15,
	8
};

extern void vesa32();
uint8_t error;
uint32_t *framebuffer;
// uint8_t i;
// uint8_t b;
uint16_t mode_width;
uint16_t mode_height;
uint8_t mode_bpp;
uint8_t mode_err;
uint32_t i;
	
void graphicstest() {
	/*_640x480x16();
	uint8_t j = 1;
	while(1) {
		for (uint32_t i = 0; i < 640*480; i++) {
			p16pixel(i%640,i/640,(j+(i/(640/16))%16));
			//if (!(i%20)) {
				//sleep(100);
			//}
		}
		j = (j + 1) & 0xF;
		sleep(500);
		double_buffer();
	}*/
	/*for (uint32_t i = mbi->framebuffer_addr; i < mbi->framebuffer_addr+(mbi->framebuffer_width*mbi->framebuffer_height*mbi->framebuffer_bpp/8); i+=4096) {
		identity_map((void *)i);
	}*/
	
	//color24 *pixels = (color24 *)mbi->framebuffer_addr;
	
	/*for (uint32_t i = 0; i < mbi->framebuffer_addr+(mbi->framebuffer_width*mbi->framebuffer_height); i++) {
		color24 pixel;
		pixel.r = i&0x000000FF;
		pixel.g = (i>>8)&0x000000FF;
		pixel.b = (i>>16)&0x000000FF;
		pixels[i]=pixel;
	}*/
	kprint("Running graphics test");
	/*if (mbi->framebuffer_bpp==24) {
		for (uint32_t y = 0; y < mbi->framebuffer_height; y++) {
			for (uint32_t x = 0; x < mbi->framebuffer_width; x++) {
				color24 c;
				c.r = x/(mbi->framebuffer_width/256);
				c.g = y/(mbi->framebuffer_height/256);
				c.b = 0;
				setpixel24(x,y,c);
			}
		}
		for(;;);
	}*/
	identity_map((void *)0x7000);
	identity_map((void *)0x8000);
	
	
	
	for (i = 0; i < 65534; i++) {
		asm("\
			mov i,%%eax;\
			mov $1,%%ch;\
			call vesa32;\
			mov %%ax,mode_width;\
			mov %%bx,mode_height;\
			mov %%cl,mode_bpp;\
			mov %%ch,%0;\
		":"=m"(mode_err)::"eax","ebx","ecx","edx");
		if ((mode_err&3)>0)
			break;
		if (mode_bpp>23) {
			printf("Available resolution: %d x %d x %d.\n",(uint32_t)mode_width,(uint32_t)mode_height,(uint32_t)mode_bpp);
			dprintf("Available resolution: %d x %d x %d.\n",(uint32_t)mode_width,(uint32_t)mode_height,(uint32_t)mode_bpp);
			sleep(1000);
		}
	}
	printf("Encountered error %d\n",(uint32_t)mode_err);
	
	for (uint8_t b = 0; b < 5; b++) {
		for (uint8_t i = 0; i < 18; i++) {
			framebuffer_width = resolutions[i*2];
			framebuffer_height = resolutions[(i*2)+1];
			framebuffer_bpp = bpp[b];
			
			printf("Test i%d b%d resolution: %d x %d x %d\n",(uint32_t)i,(uint32_t)b,(uint16_t)framebuffer_width,(uint16_t)framebuffer_height,(uint32_t)framebuffer_bpp);
			dprintf("Test i%d b%d resolution: %d x %d x %d\n",(uint32_t)i,(uint32_t)b,(uint16_t)framebuffer_width,(uint16_t)framebuffer_height,(uint32_t)framebuffer_bpp);
			
			asm("\
				mov framebuffer_width,%%eax;\
				mov framebuffer_height,%%ebx;\
				mov framebuffer_bpp,%%cl;\
				mov $0,%%ch;\
				call vesa32;\
				mov %%al,error;\
				mov %%ebx,framebuffer;\
			":::"eax","ebx","ecx");
			
			kprint("Done with BIOS!");
			dprintf("Final resolution: %d x %d x %d\n",(uint16_t)framebuffer_width,(uint16_t)framebuffer_height,(uint8_t)framebuffer_bpp);
			
			if (!error) {
				dprintf("Error is zero\n");
			} else {
				kerror("Error is NONZERO!!!");
				dprintf("%d\n",(uint32_t)error);
			}
			if (!framebuffer) {
				kerror("Framebuffer is zero!");
			} else {
				dprintf("Framebuffer seems ok\n");
			}
			dprintf("%#\n",(uint64_t)(uint32_t)framebuffer);
			
			init_pit(1000);
			
			if (!error && framebuffer) {
				framebuffer_addr = framebuffer;
				framebuffer_pitch = framebuffer_width*framebuffer_bpp/8;
				
				for (uint32_t i = (uint32_t)framebuffer_addr; i < (uint32_t)framebuffer_addr+(framebuffer_height*framebuffer_pitch); i+=4096) {
					identity_map((void *)i);
				}
				
				doublebuffer = alloc_page(((framebuffer_height*framebuffer_pitch)/4096)+1);
				
				if (framebuffer_bpp==24||framebuffer_bpp==32) {
					if (framebuffer_bpp==24) {
						for (uint32_t y = 0; y < framebuffer_height; y++) {
							for (uint32_t x = 0; x < framebuffer_width; x++) {
								color24 c;
								c.r = x/(framebuffer_width/256);
								c.g = y/(framebuffer_height/256);
								c.b = 0;
								setpixel24(x,y,c);
							}
						}
					} else if (framebuffer_bpp==32) {
						for (uint32_t y = 0; y < framebuffer_height; y++) {
							for (uint32_t x = 0; x < framebuffer_width; x++) {
								color32 c;
								c.r = x/(framebuffer_width/256);
								c.g = y/(framebuffer_height/256);
								c.b = 0;
								c.a = 0;
								setpixel32(x,y,c);
							}
						}
					}
					
					syncvideo();
					
					if (framebuffer_bpp==24) {
						for (uint32_t y = 0; y < framebuffer_height; y++) {
							for (uint32_t x = 0; x < framebuffer_width; x++) {
								color24 c;
								c.r = x/(framebuffer_width/256);
								c.b = y/(framebuffer_height/256);
								c.g = 0;
								setpixel24(x,y,c);
							}
						}
					} else if (framebuffer_bpp==32) {
						for (uint32_t y = 0; y < framebuffer_height; y++) {
							for (uint32_t x = 0; x < framebuffer_width; x++) {
								color32 c;
								c.r = x/(framebuffer_width/256);
								c.b = y/(framebuffer_height/256);
								c.g = 0;
								setpixel32(x,y,c);
							}
						}
					}
					
					color24 front;
					memset(&front,0xFF,3);
					print_string(10,10,"Hello World!",front);
					print_string(10,26," !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~",front);
					syncvideo();
				}
				
				//memcpy(framebuffer_addr,doublebuffer,(framebuffer_height*framebuffer_pitch));
				
				
				free_page(doublebuffer,((framebuffer_height*framebuffer_pitch)/4096)+1);
				for (uint32_t i = (uint32_t)framebuffer_addr; i < (uint32_t)framebuffer_addr+(framebuffer_width*framebuffer_height*framebuffer_bpp/8); i+=4096) {
					unmap_vaddr((void *)i);
				}
			}
			sleep(1000);
		}
	}
	
	kprint("All tests are complete");
}

