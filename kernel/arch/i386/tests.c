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

//Old GDT Code

	/*
	 * Welcome to the Global Descriptor Table (GDT)
	 *
	 * First off, we have previously calculated the total memory that we have
	 *
	 *
	 */
	
	/*These two will be at zero otherwise the GDT goes crazy and triple faults
	 *Besides, the kernel can have access to as much memory as it wants
	 *Lastly, the kernel won't need as much memory as the userspace will, so we
	 *can squeeze both the code and the data in the 4 MiB we provide.*/
    gdtEntries[1] = gdt_encode(0, total_mem/2, (GDT_CODE_PL0));
    gdtEntries[2] = gdt_encode(0, total_mem/2, (GDT_DATA_PL0)); 
	
	//Userspace will be above the kernel. This isn't the most efficient, but oh well
    gdtEntries[3] = gdt_encode((total_mem/2)/2, total_mem/2, (GDT_CODE_PL3));
    gdtEntries[4] = gdt_encode(total_mem/2, total_mem/2, (GDT_DATA_PL3));

	gdtEntries[5] = gdt_encode(&tss,sizeof(tss_entry_t), 0x0489); //I don't exactly understand, but I do know the tss needs this type
	
//Entering usermode and tests
	uint8_t function[2] = {0xEB, 0xFE};
	
	kprint("Entering usermode! Hold tight!");

	enter_usermode();
	//We're in usermode (hopefully)! Lets see what we can do
	
	//Test syscalls
	char * teststring = "Hello User Mode World! Test number 123 = %d.\n";
	int testnum = 123;
	asm volatile("mov $0, %%eax; mov %2, %%ecx; lea (%1),%%ebx; int $0x80" : : "a" (0), "r" ((uint32_t)teststring), "r" (testnum));
	
	teststring = "Don't worry about the error below. It means that usermode is working.\n";
	asm volatile("mov $0, %%eax; lea (%1),%%ebx; int $0x80" : : "a" (0), "r" ((uint32_t)teststring));
	
	//This will cause a Page Fault if it works, otherwise it actually prints (which is bad)
	printf("Uh oh. Usermode isn't working!");
