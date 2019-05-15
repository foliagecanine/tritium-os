#include <kernel/fs.h>

//Thanks to https://www.win.tue.nl/~aeb/linux/fs/fat/fat-1.html for the fat12 details
//along with http://www.brokenthorn.com/Resources/OSDev22.html for when I get stuck
//However, I wrote this wholly myself

typedef struct {
	uint8_t OEMName[8];
	uint16_t BytesPerSector;
	uint8_t SectorsPerCluster;
	uint16_t NumReservedSectors;
	uint8_t NumFATs;
	uint16_t NumRootDirectoryEntries;
	uint16_t NumTotalSectors;
	uint8_t MediaDescriptorType;
	uint16_t NumSectorsPerFAT;
	uint16_t NumSectorsPerTrack;
	uint16_t NumHeads;
	uint16_t NumHiddenSectors;
} __attribute__ ((packed)) BPB, *PBPB; //Hehe: ptr BIOS parameter block
//These must be packed or else the compiler will space these out, making the pointer invalid
//When it was not packed, the OEMName listed as SWIN4.1? where ? is an unknown unprintable value.

typedef struct {
	uint8_t StartCode[3];
	BPB BiosParameterBlock;
	uint8_t Bootstrap[480]; //Funny thing is brokenthorn has anther BPB and reduces the size of the bootstrap... No clue, but I'll follow the win.tue.nl one.
	uint16_t Signature;
} __attribute__ ((packed)) BOOTSECT, *PBOOTSECT;

void print_fat12_values(uint8_t drive_num) {
	if (!drive_exists(drive_num))
		return;
	
	uint8_t read[512];
	read_sector_lba(drive_num,0,read);
	PBOOTSECT bootsect = (PBOOTSECT *)read;
	BPB bpb = bootsect->BiosParameterBlock;
	
	//Print Start Code
	uint8_t *startcode = (uint8_t *)bootsect->StartCode;
	printf("Start code: %# %# %#\n",(uint64_t)startcode[0],(uint64_t)startcode[1],(uint64_t)startcode[2]);
	
	//Print OEM Name
	char OEMName[9];
	memcpy(OEMName,bpb.OEMName,8);
	OEMName[8]="\0";
	printf("OEMName: %s\n", OEMName);
	
	//Other stuff
	printf("Bytes Per Sector: %d\n",(uint64_t)bpb.BytesPerSector);
	printf("Sectors Per Cluster: %d\n",(uint64_t)bpb.SectorsPerCluster);
	printf("Number of FATs: %d\n",(uint64_t)bpb.NumFATs);
	printf("Number of Root Directory Entries: %d\n",(uint64_t)bpb.NumRootDirectoryEntries);
	printf("Number of Total Sectors: %d\n",(uint64_t)bpb.NumTotalSectors);
	printf("Media Descriptor Type: %#\n",(uint64_t)bpb.MediaDescriptorType);
	printf("Number of Sectors Per FAT: %d\n",(uint64_t)bpb.NumSectorsPerFAT);
	printf("Number of Sectors Per Track: %d\n",(uint64_t)bpb.NumSectorsPerTrack);
	printf("Number of Heads: %d\n",(uint64_t)bpb.NumHeads);
	printf("Number of Hidden Sectors: %d\n",(uint64_t)bpb.NumHiddenSectors);
	printf("Signature: %#\n",(uint64_t)bootsect->Signature);
}

//Listing all the things as shown in https://forum.osdev.org/viewtopic.php?f=1&t=26639
_Bool detect_fat12(uint8_t drive_num) {
	uint8_t read[512];
	read_sector_lba(drive_num,0,read);
	PBOOTSECT bootsect = (PBOOTSECT *)read;
	BPB bpb = bootsect->BiosParameterBlock;
	
	if (bootsect->Signature!=0xAA55)
		return false;

	if (bpb.BytesPerSector%2!=0||!(bpb.BytesPerSector>=512)||!(bpb.BytesPerSector<=4096))
		return false;
	
	if (bpb.MediaDescriptorType!=0xf0&&!(bpb.MediaDescriptorType>=0xf8))
		return false;

	if (bpb.NumFATs==0)
		return false;
	
	if (bpb.NumRootDirectoryEntries==0)
		return false;
	
	//This is probably enough. If it is just a random string of digits, we'll probably have broken it by now.
	return true;
}
