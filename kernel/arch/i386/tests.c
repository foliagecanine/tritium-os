//These are all tests. These are not built into the actual project, just copied and pasted here rather than commented out

//Check if drive is formatted FAT12
if (!drive_exists(0))
	printf("No drive in drive 0.\n");
if (detect_fat12(0)) {
	printf("Drive 0 is formatted FAT12\n");
} else {
	printf("Drive 0 is NOT formatted FAT12\n");
}
	
//Try to mount drive 0
printf("Attempting to mount drive 0.\n");
uint8_t mntErr = mountDrive(0);
if (!mntErr) {
	printf("Successfully mounted drive!\n");
} else {
	printf("Mount error %d\n",(uint32_t)mntErr);
}

//Test invalid opcode
int a;
a = 1/0;
printf("%d\n",a);*/

//Test division by zero
asm volatile(	
"mov $0, %edx\n"
"mov $0, %eax\n"
"mov $0, %ecx\n"
"div %ecx"
);

//Test file size
char fame[12];
strcpy(fame,"A:/testfldr");
FILE myfile = fopen(fame,"r+");
printf("Size of A:/testfldr is %d\n",myfile.size);

//Test keyboard
for (;;) {
	char c = getchar();
	if (c) {
		printf("%c",c);
	} else if (getkey()==14) {
		terminal_backup();
		putchar(' ');
		terminal_backup();
	}
}