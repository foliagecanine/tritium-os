#include <fs/fat12.h>

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
} __attribute__ ((packed)) BPB, *PBPB;

typedef struct {
	uint8_t StartCode[3];
	BPB BiosParameterBlock;
	uint8_t Bootstrap[480];
	uint16_t Signature;
} __attribute__ ((packed)) BOOTSECT, *PBOOTSECT;

typedef struct {
	uint8_t Filename[8];
	uint8_t Extension[3];
	uint8_t FileAttributes;
	uint8_t Reserved;
	uint8_t TimeCreatedMillis;
	uint16_t TimeCreated;
	uint16_t DateCreated;
	uint16_t DateLastAccessed;
	uint16_t FirstClusterHi;
	uint16_t LastModificationTime;
	uint16_t LastModificationDate;
	uint16_t FirstClusterLocation;
	uint32_t FileSize;
} __attribute__ ((packed)) FAT12DIR, *PFAT12DIR;

int findCharInArray(char * array, char c) {
	for (uint8_t i = 0; i < strlen(array); i++) {
		if (array[i]==c) {
			return i;
		}
	}
	return -1;
}

char intToChar(uint8_t num) {
	const char nums[] = {'0','1','2','3','4','5','6','7','8','9'};
	if (num<10) {
		return nums[num];
	} else {
		return 0;
	}
}

void print_fat12_values(uint8_t drive_num) {
	if (!drive_exists(drive_num))
		return;
	
	uint8_t read[512];
	ahci_read_sector(drive_num,0,read);
	PBOOTSECT bootsect = (PBOOTSECT)read;
	BPB bpb = bootsect->BiosParameterBlock;
	
	//Print Start Code
	uint8_t *startcode = (uint8_t *)bootsect->StartCode;
	printf("Start code: %# %# %#\n",(uint64_t)startcode[0],(uint64_t)startcode[1],(uint64_t)startcode[2]);
	
	//Print OEM Name
	char OEMName[9];
	memcpy(OEMName,bpb.OEMName,8);
	OEMName[8]=0;
	printf("OEMName: %s\n", OEMName);
	
	//Other stuff
	printf("Bytes Per Sector: %d\n",(uint32_t)bpb.BytesPerSector);
	printf("Sectors Per Cluster: %d\n",(uint32_t)bpb.SectorsPerCluster);
	printf("Number of FATs: %d\n",(uint32_t)bpb.NumFATs);
	printf("Number of Root Directory Entries: %d\n",(uint32_t)bpb.NumRootDirectoryEntries);
	printf("Number of Total Sectors: %d\n",(uint32_t)bpb.NumTotalSectors);
	printf("Media Descriptor Type: %#\n",(uint64_t)bpb.MediaDescriptorType);
	printf("Number of Sectors Per FAT: %d\n",(uint32_t)bpb.NumSectorsPerFAT);
	printf("Number of Sectors Per Track: %d\n",(uint32_t)bpb.NumSectorsPerTrack);
	printf("Number of Heads: %d\n",(uint32_t)bpb.NumHeads);
	printf("Number of Hidden Sectors: %d\n",(uint32_t)bpb.NumHiddenSectors);
	printf("Signature: %#\n",(uint64_t)bootsect->Signature);
}

//Listing all the things as shown in https://forum.osdev.org/viewtopic.php?f=1&t=26639
bool detect_fat12(uint8_t drive_num) {
	uint8_t read[512];
	ahci_read_sector(drive_num,0,read);
	PBOOTSECT bootsect = (PBOOTSECT)read;
	BPB bpb = bootsect->BiosParameterBlock;
	
	if (bootsect->Signature!=0xAA55) {
		//printf("Signature error.\n");
		return false;
	}

	if (bpb.BytesPerSector%2!=0||!(bpb.BytesPerSector>=512)||!(bpb.BytesPerSector<=4096)) {
		//printf("Illegal bytes per sector.\n");
		return false;
	}
	
	if (bpb.MediaDescriptorType!=0xf0&&!(bpb.MediaDescriptorType>=0xf8)) {
		//printf("Illegal MDT.\n");
		return false;
	}

	if (bpb.NumFATs==0) {
		return false;
		//printf("No FATs.\n");
	}
	
	if (bpb.NumRootDirectoryEntries==0) {
		//printf("No RDEs.\n");
		return false;
	}
	
	//This is probably enough. If it is just a random string of digits, we'll probably have broken it by now.
	//printf("Success in %d.\n",(uint32_t)drive_num);
	return true;
}

/*
 * This function should probably be optimized!
 */
void LongToShortFilename(char * longfn, char * shortfn) {
	// Longfilename.extension -> LONGFI~6EXT, textfile.txt -> TEXTFILETXT, short.txt -> SHORT   TXT
	memset(shortfn,' ',11); //Fill with spaces
	
	//First check for . and ..
	if (strcmp(longfn,".")) {
		strcpy(shortfn,".          ");
		return;
	}
	if (strcmp(longfn,"..")) {
		strcpy(shortfn,"..         ");
		return;
	}
	
	//Then do the rest
	int locOfDot = findCharInArray(longfn,'.');
	if (locOfDot>8) {
		memcpy(shortfn,longfn,6);
		shortfn[6]='~';
		if ((locOfDot-6)>9) {
			shortfn[7]='~';
		} else {
			shortfn[7]=intToChar(locOfDot-6);
		}
	} else {
		if (locOfDot!=-1) { //If there is no dot then just copy the whole thing (up to 8).
			memcpy(shortfn,longfn,locOfDot);
			for (uint8_t i = strlen(longfn)-4; i < 9; i++) {
				shortfn[i]=' ';
			}
		} else if (strlen(longfn)<9) {
			memcpy(shortfn,longfn,strlen(longfn));
		} else {
			memcpy(shortfn,longfn,6);
			shortfn[6]='~';
			if ((strlen(longfn)-6)>9) {
				shortfn[7]='~';
			} else {
				shortfn[7]=intToChar(strlen(longfn)-6);
			}
		}
	}
	
	//Check for extension
	if (locOfDot!=-1) {
		//Yes extension. Copy up to the first 3 letters. If more than 3 do this: extens -> e~5
		int extLen = strlen(longfn)-locOfDot-1;
		
		if (extLen>0) 
			shortfn[8]=longfn[locOfDot+1];
		
		if (extLen>1&&extLen<4) 
			shortfn[9]=longfn[locOfDot+2];
		
		if (extLen>2&&extLen<4)
			shortfn[10]=longfn[locOfDot+3];
		
		if (extLen>=4) {
			shortfn[9]='~';
			if ((extLen-1)>9) {
			shortfn[10]='~';
			} else {
				shortfn[10]=intToChar(extLen-1);
			}
		}
		
		shortfn[11] = 0; //End string
	} else {
		//No extension. Just put 3 spaces.
		shortfn[8]=' '; shortfn[9]=' '; shortfn[10] = ' ';
		shortfn[11] = 0; //End string
	}
	
	//Uppercase our name. 8.3 only stores uppercase (not counting LFN; not going to do LFN for a while)
	for (uint8_t i = 0; i < 12; i++) {
		shortfn[i] = toupper(shortfn[i]);
	}
	
	//Add any neccesary padding
	/* if (shortfn[0]!=0) {
		for (uint8_t i = 0; i<11; i++) {
			if (shortfn[i]==0)
				shortfn[i] = ' ';
		}
	} */
}

FSMOUNT MountFAT12(uint8_t drive_num) {
	//Get neccesary details. We don't need to check whether this is FAT12 because it is already done in file.c.
	uint8_t read[512];
	ahci_read_sector(drive_num,0,read);
	PBOOTSECT bootsect = (PBOOTSECT)read;
	BPB bpb = bootsect->BiosParameterBlock;
	
	//Setup basic things
	FSMOUNT fat12fs;
	strcpy(fat12fs.type,"FAT12");
	fat12fs.type[5]=0;
	fat12fs.drive = drive_num;
	
	//Set the mount part of FSMOUNT to our FAT12_MOUNT
	FAT12_MOUNT *fat12mount = (FAT12_MOUNT *)alloc_page(1);
	
	fat12mount->MntSig = 0xAABBCCDD;
	fat12mount->NumTotalSectors = bpb.NumTotalSectors;
	fat12mount->FATOffset = 1;
	fat12mount->NumRootDirectoryEntries = bpb.NumRootDirectoryEntries;
	fat12mount->FATEntrySize = 12;
	fat12mount->RootDirectoryOffset = (bpb.NumFATs * bpb.NumSectorsPerFAT) + 1;
	fat12mount->RootDirectorySize = (bpb.NumRootDirectoryEntries * 32) / bpb.BytesPerSector;
	fat12mount->FATSize = bpb.NumSectorsPerFAT;
	fat12mount->SectorsPerCluster = bpb.SectorsPerCluster;
	fat12mount->SystemAreaSize = bpb.NumReservedSectors + (bpb.NumFATs*bpb.NumSectorsPerFAT+((32*bpb.NumRootDirectoryEntries)/bpb.BytesPerSector));
	
	//Store mount info
	fat12fs.mount = fat12mount;
	
	//Enable mount
	fat12fs.mountEnabled = true;
	
	return fat12fs;
}

uint64_t getLocationFromCluster(uint32_t clusterNum,FAT12_MOUNT fm) {
	return (uint64_t)((clusterNum-2)*512)+(fm.NumRootDirectoryEntries*32)+(fm.RootDirectoryOffset*512);
}

uint16_t getClusterValue(uint8_t * FAT, uint32_t cluster) {
	uint16_t value = 0;
	if (cluster%2==0) {
		value = (FAT[1+((3*cluster)/2)]&0x0F)<<8;
		value += FAT[(3*cluster)/2];
		return value;
	} else {
		value = (FAT[(3*cluster)/2]&0xF0)<<4;
		value += FAT[1+((3*cluster)/2)];
		return value;
	}
}

void FAT12_print_folder(uint32_t location, uint32_t numEntries, uint8_t drive_num) {
	uint8_t *read = alloc_page(((numEntries*32)/4096)+1);
	memset(read,0,numEntries*32);
	
	for (uint8_t i = 0; i < (512)/512; i++) {
		uint8_t derr = ahci_read_sector(drive_num, (location-1)/512+i, read+(i*512));
		if (derr) {
			printf("Drive error: %d!",derr);
			free_page(read,((numEntries*32)/4096)+1);
			return;
		}
	}
		
	char drivename[12];
	memset(&drivename,0,12);
	memcpy(drivename,read,8);
	if (read[9]!=' ') {
		drivename[8]='.';
		memcpy(drivename+9,read+8,3);
	}
	drivename[11] = 0;
	
	uint8_t *reading = (uint8_t *)read;
	printf("Listing files/folders in current directory:\n");
	for (unsigned int i = 0; i < numEntries; i++) {
		if (!(reading[11]&0x08||reading[11]&0x02||reading[0]==0||reading[0]==0xE5)) {
			for (uint8_t j = 0; j < 11; j++) {
				if (j==8&&reading[j]!=' ')
					printf(".");
				if (reading[j]!=0x20)
					printf("%c",reading[j]);
				if (reading[11]&0x10&&j==10)
					printf("/");
			}
			//int32_t nextCluster = (reading[27] << 8) | reading[26];
			printf(" [%d]");
			printf("\n");
		}
		reading+=32;
	}
	printf("--End of directory----------------------\n");
	free_page(read,((numEntries*32)/4096)+1);
}

/* 
 * A quick note on modes:
 *
 * Bit 0 = read
 * Bit 1 = write
 */

//This function is recursive! It will continue opening files & folders until error or success
FILE FAT12_fopen(uint32_t location, uint32_t numEntries, char *filename, uint8_t drive_num, FAT12_MOUNT fm, uint8_t mode) {
	FILE retFile;
	char *searchpath = filename+1;
	char searchname[((int)strchr(searchpath,'/')-(int)searchpath)+1];
	#pragma GCC diagnostic ignored "-Wint-conversion"
	memcpy(searchname,searchpath,(strchr(searchpath,'/')-(int)searchpath));
	searchname[((int)strchr(searchpath,'/')-(int)searchpath)] = 0;
	searchpath+=((int)strchr(searchpath,'/')-(int)searchpath);
	
	char shortfn[12];
	LongToShortFilename(searchname, shortfn); //Get the 8.3 name of the file/folder we are looking for
	
	//If we added a /, don't bother looking for NULL. Instead, return our current location.
	if (strcmp(shortfn,"           ")) {
		retFile.valid = true;
		retFile.location = (uint64_t)location;
		retFile.size = numEntries; //This is definately a directory.
		retFile.directory = true;
		return retFile;
	}
	
	uint8_t *read = alloc_page(((numEntries*32)/4096)+1);; //See free below
	memset(read,0,numEntries*32);
	
	for (uint8_t i = 0; i < (numEntries*32)/512; i++) {
		printf("Loc: %# : %#\n",(uint64_t)(location-1)/512+i, (uint64_t)read+(i*512));
		uint8_t derr = ahci_read_sector(drive_num, (location-1)/512+i, read+(i*512));
		if (derr) {
			retFile.valid = false;
			return retFile;
		}
	}
	
	printf("End of read\n");
	
	char drivename[12];
	memcpy(drivename,read,8);
	if (read[9]!=' ') {
		drivename[8]='.';
		memcpy(drivename+9,read+8,3);
	}
	drivename[11] = 0;
		
	uint8_t *reading = (uint8_t *)read;
	_Bool success = false;
	for (unsigned int i = 0; i < numEntries; i++) {
		if (!(reading[11]&0x08||reading[11]&0x02||reading[0]==0)) {
			char testname[12];
			memcpy(testname,reading,11);
			testname[11] = 0;
			if (strcmp(testname,shortfn)) {
				success = true;
				break;
			}
		}
		reading+=32;
	}
	
	if (success) {
		if (searchpath&&reading[11]&0x10) {
			uint16_t nextCluster = (reading[27] << 8) | reading[26];
			free_page(read,((numEntries*32)/4096)+1); //This way we don't use so much space. We aren't going to use this data any more.
			return FAT12_fopen((fm.SystemAreaSize+((nextCluster-2)*fm.SectorsPerCluster))*512+1,16,searchpath,drive_num,fm,mode);
		} else {
			int32_t nextCluster = (reading[27] << 8) | reading[26];
			retFile.clusterNumber = nextCluster;
			retFile.valid = true;
			retFile.location = getLocationFromCluster(nextCluster,fm);
			uint32_t size = (reading[31]<<24) | (reading[30]<<16) | (reading[29]<<8) | reading[28];
			retFile.size = (uint64_t)size;
			if (mode&1) {
				retFile.writelock = true;
			} else {
				retFile.writelock = false;
			}
			if (reading[11]&0x10) {
				retFile.directory = true;
			} else {
				retFile.directory = false;
			}
			free_page(read,((numEntries*32)/4096)+1);
			return retFile;
		}
	} else {
		free_page(read,((numEntries*32)/4096)+1);
		retFile.valid = false;
		return retFile;
	}
}

void FAT12_fread(FILE *file, char *buf, uint32_t start, uint32_t len, uint8_t drive_num) {
	if (!file)
		return;
	FAT12_MOUNT fm = *(FAT12_MOUNT *)getDiskMount(file->mountNumber).mount;
	uint8_t FAT[fm.FATSize*512];
	for (uint8_t i = 0; i < fm.FATSize; i++)
		ahci_read_sector(drive_num,i+1,&FAT[i*512]);
	uint16_t rCluster = file->clusterNumber;
	uint32_t curLen = len;
	uint32_t curLoc = start;
	uint32_t curDiskLoc = 0;
	char read[512];
	while (curLen>0) {
		if ((curLoc-curDiskLoc)<512) {
			ahci_read_sector(drive_num,getLocationFromCluster(rCluster,fm)/512,(uint8_t *)read);
			uint32_t amt = (curLen>512?512:curLen)-(curLoc%512);
			memcpy(buf+curLoc,&read[curLoc%512],amt);
			curLen-=amt;
			curLoc+=amt;
		}
		curDiskLoc+=512;
		rCluster = getClusterValue(FAT,rCluster);
		if (rCluster>=0xFF0) {
			break;
		}
	}
}
