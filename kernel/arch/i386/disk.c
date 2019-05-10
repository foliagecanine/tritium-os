#include <kernel/fs.h>

/*Error codes:
 *0=Success
 *1=Illegal drive number
 *2=Drive not found
 *3=Unknown drive type
 *4=Unknown error
 */

#define ATA_STATUS_ERR 		0x01
#define ATA_STATUS_BSY 		0x80
#define ATA_STATUS_DRQ 		0x08
#define ATA_STATUS_DRDY		0x40
#define ATA_STATUS_DF 		0x20

typedef struct {
	_Bool				exists;
	uint16_t 			iobase;
	unsigned char 		channel;
	unsigned char		type;
	unsigned char		signature;
	unsigned char		features;
	unsigned int		cmdsets;
	unsigned int 		size;
	unsigned char 		model;
} ata_drive_t;

ata_drive_t ata_drives[4];

void small_delay(uint16_t iobase) {
	for(int i = 0; i < 4; i++) //400 ns delay
      inb(iobase+7);
}

//Based off of https://wiki.osdev.org/PCI_IDE_Controller
uint8_t poll_ide(uint16_t iobase, _Bool adv) {
   small_delay(iobase);
   while (inb(iobase+7)& ATA_STATUS_BSY) //wait for busy to clear
 
   if (adv) { //need more info?
      uint8_t state = inb(iobase+7);
      if (state & ATA_STATUS_ERR)
         return 2;
      if (state & ATA_STATUS_DF)
         return 1;
      if ((state & ATA_STATUS_DRQ) == 0)
         return 3;
   }
   return 0;
}

void detect_devices(ata_drive_t* drive) {
	drive->exists = 0;
	drive->type = 0;
	outb(drive->iobase+6,0xA0+(0x10*drive->channel)); //Select drive
	small_delay(drive->iobase);
	
	outb(drive->iobase+7,0xEC);
	
	small_delay(drive->iobase);
	
	if (inb(drive->iobase+7)==0) return;
	
	_Bool err = false;
	while(1) {
		uint8_t status = inb(drive->iobase+7);
		if ((status & ATA_STATUS_ERR)) {err = true; break;} // If Err, Device is not ATA.
		if ((!(status & ATA_STATUS_BSY))&&(status & ATA_STATUS_DRDY)) {break;} // Everything is right.
    }
	
    if (err) {
		uint8_t cl = inb(drive->iobase+4);
		uint8_t ch = inb(drive->iobase+5);

		if (cl == 0x14 && ch ==0xEB)
			drive->type = 1;
		else if (cl == 0x69 && ch == 0x96)
			drive->type = 1;
		else
		   return; // Unknown Type (may not be a device).

		outb(drive->iobase+7, 0xA1);
		small_delay(drive->iobase);
	 }
	 
	 uint8_t buffer[2048];
	 
	 insl(drive->iobase,(unsigned int)buffer,128);
	 
	 drive->exists = 1;
	 drive->signature = *((unsigned short *)(buffer));
	 drive->features = *((unsigned short *)(buffer + 98));
	 drive->cmdsets = *((unsigned short *)(buffer + 164));
	 
	 if (drive->cmdsets & (1 << 26))
		drive->size   = *((unsigned int *)(buffer + 200));
	 else
		drive->size   = *((unsigned int *)(buffer + 120));

	char *model = (char *)drive->model;
	 for(int k = 0; k < 40; k += 2) {
		model[k] = buffer[54 + k + 1];
		model[k + 1] = buffer[54 + k];}
	 model[40] = 0; // Terminate String.
	 
     if (drive->exists) {
        printf("Found %s Drive %dMB (%dGB) - %s\n",
           (const char *[]){"ATA", "ATAPI"}[drive->type],	/* Type */
		   drive->size /1024 /2,										/* Size */
           drive->size / 1024 / 1024 / 2,							/* Size again */
           drive->model);
     }
}

void init_ata() {
	kprint("Searching for ATA Drives. Details listed below.");
	ata_drives[0].iobase = 0x1F0;
	ata_drives[0].channel = 0;
	
	ata_drives[1].iobase = 0x1F0;
	ata_drives[1].channel = 1;
	
	ata_drives[2].iobase = 0x170;
	ata_drives[2].channel = 0;
	
	ata_drives[3].iobase = 0x170;
	ata_drives[3].channel = 1;
	
	ata_drives[4].iobase = 0x1E8;
	ata_drives[4].channel = 0;
	
	ata_drives[5].iobase = 0x1E8;
	ata_drives[5].channel = 1;
	
	ata_drives[6].iobase = 0x168;
	ata_drives[6].channel = 0;
	
	ata_drives[7].iobase = 0x168;
	ata_drives[7].channel = 1;
	
	detect_devices(&ata_drives[0]);
	detect_devices(&ata_drives[1]);
	detect_devices(&ata_drives[2]);
	detect_devices(&ata_drives[3]);
	detect_devices(&ata_drives[4]);
	detect_devices(&ata_drives[5]);
	detect_devices(&ata_drives[6]);
	detect_devices(&ata_drives[7]);
}

//VERY based off of http://www.rohitab.com/discuss/topic/39244-read-hdd-sector-using-in-instruction/
uint8_t read_sectors_lba(uint8_t drive_num, uint32_t lba_start, uint8_t num_sectors, uint8_t *dest) {
	uint16_t iobase;
	uint16_t max_read;
	uint16_t *buffer = (uint16_t *)dest;
	uint8_t drive = 0x40;
	
	switch(drive_num){
        case 0: case 1: iobase = 0x1F0; break;
        case 2: case 3: iobase = 0x170; break;
        case 4: case 5: iobase = 0x1E8; break; //Most 
        case 6: case 7: iobase = 0x168; break;
        default: return 1;
    }
	
	//Check for drive existance
		//Nothing here yet...
	//End of check
	
	if (drive_num%2) {
		drive |=0x10;
	}
	
	outb(iobase + 2, num_sectors);
    outb(iobase + 3, (uint8_t)((lba_start & 0x000000FF))); //First byte of LBA
    outb(iobase + 4, (uint8_t)((lba_start & 0x0000FF00) >>  8)); //Second
    outb(iobase + 5, (uint8_t)((lba_start & 0x00FF0000) >> 16)); //Third
    outb(iobase + 6, (uint8_t)((lba_start & 0x0F000000) >> 24) | drive); //Finally the last nibble (LBA48 means only 48 bits which is 3 1/2 bytes)
    outb(iobase + 7, 0x20); //Start reading
	
	while (inb(iobase + 7) & 0x80); //Wait till its not busy
	
	max_read = (num_sectors * (512 / 2));
	
	insw(iobase,dest,max_read);
	
	return 0;
}

uint8_t read_sector_lba(uint8_t drive_num, uint32_t lba, uint8_t *dest) {
	return read_sectors_lba(drive_num,lba,1,dest);
}
