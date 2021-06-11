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

void print_fat12_values(uint8_t drive_num) {
	if (!drive_exists(drive_num))
		return;
	
	uint8_t read[512];
	ahci_read_sector(drive_num,0,read);
	PBOOTSECT bootsect = (PBOOTSECT)read;
	BPB bpb = bootsect->BiosParameterBlock;
	
	//Print Start Code
	uint8_t *startcode = (uint8_t *)bootsect->StartCode;
	printf("Start code: %X %X %X\n",startcode[0],startcode[1],startcode[2]);
	
	//Print OEM Name
	char OEMName[9];
	memcpy(OEMName,bpb.OEMName,8);
	OEMName[8]=0;
	printf("OEMName: %s\n", OEMName);
	
	//Other stuff
	printf("Bytes Per Sector: %u\n",bpb.BytesPerSector);
	printf("Sectors Per Cluster: %u\n",bpb.SectorsPerCluster);
	printf("Number of FATs: %u\n",bpb.NumFATs);
	printf("Number of Root Directory Entries: %u\n",bpb.NumRootDirectoryEntries);
	printf("Number of Total Sectors: %u\n",bpb.NumTotalSectors);
	printf("Media Descriptor Type: %X\n",bpb.MediaDescriptorType);
	printf("Number of Sectors Per FAT: %u\n",bpb.NumSectorsPerFAT);
	printf("Number of Sectors Per Track: %u\n",bpb.NumSectorsPerTrack);
	printf("Number of Heads: %u\n",bpb.NumHeads);
	printf("Number of Hidden Sectors: %u\n",bpb.NumHiddenSectors);
	printf("Signature: %X\n",bootsect->Signature);
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
	
	if (bpb.NumSectorsPerFAT!=9) {
		//printf("Number of sectors per FAT incorrect\n");
		return false;
	}
	
	//This is probably enough. If it is just a random string of digits, we'll probably have broken it by now.
	//printf("Success in %u.\n",drive_num);
	return true;
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
		value = (FAT[1+(3*cluster)/2])<<4;
		value += (FAT[((3*cluster)/2)]&0xF0)>>4;
		return value;
	}
}

void FAT12_print_folder(uint32_t location, uint32_t numEntries, uint8_t drive_num) {
	uint8_t *read = alloc_sequential(((numEntries*32)/4096)+1);
	memset(read,0,numEntries*32);
	
	uint8_t derr = ahci_read_sector(drive_num, location/512, read);
	if (derr) {
		printf("Drive error: %$!",derr);
		free_page(read,((numEntries*32)/4096)+1);
		return;
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
			uint32_t nextCluster = (reading[27] << 8) | reading[26];
			uint32_t size = *(uint32_t *)&reading[28];
			printf(" [0x%lX+%u]",(uint64_t)((nextCluster-2)*512)+(224*32)+(19*512),size);
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
	char searchname[13];
	char *searchpath = filename+1;
	if (strlen(filename)>0) {
		memcpy(searchname,searchpath,((int)strchr(searchpath,'/')-(int)searchpath));
		searchname[((int)strchr(searchpath,'/')-(int)searchpath)] = 0;
		searchpath+=((int)strchr(searchpath,'/')-(int)searchpath);
	} else {
		strcpy(searchname,"");
	}
	
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
	uint32_t num_pages = ((numEntries*32)/4096)+1;
	uint8_t *read = alloc_sequential(num_pages); //See free below
	memset(read,0,num_pages*4096);
	
	uint8_t derr = ahci_read_sectors(drive_num, (location/512), (numEntries*32)/512, read);
	if (derr) {
		retFile.valid = false;
		return retFile;
	}
	
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

uint8_t FAT12_fread(FILE *file, char *buf, uint32_t start, uint32_t len, uint8_t drive_num) {
	if (!file)
		return 1;
	FAT12_MOUNT fm = *(FAT12_MOUNT *)getDiskMount(file->mountNumber).mount;
	uint8_t *FAT = (uint8_t *)alloc_page(((fm.FATSize*512)/4096)+1);
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
			uint32_t amt = curLen > 512 ? 512 : curLen;
			memcpy(buf+(curLoc-start),read+(curLoc%512),amt);
			curLen -= amt;
			curLoc = (curLoc + 512) - (curLoc % 512);
		}
		curDiskLoc+=512;
		rCluster = getClusterValue(FAT,rCluster);
		if (rCluster>=0xFF0) {
			break;
		}
	}
	free_page(FAT,((fm.FATSize*512)/4096)+1);
	return 0;
}

FILE FAT12_readdir(FILE *file, char *buf, uint32_t n, uint8_t drive_num) {
	FILE retfile;
	memset(&retfile,0,sizeof(FILE));
	
	if (!file)
		return retfile;
	if (n>31)
		return retfile;
	
	char read[512];
	memset(read,0,512);
	ahci_read_sector(drive_num,file->location/512,(uint8_t *)read);
	FAT12DIR dir_entry = *(PFAT12DIR)(read+(32*n));
	
	if (dir_entry.FileAttributes&0x08||dir_entry.FileAttributes&0x02||dir_entry.Filename[0]==0||dir_entry.Filename[0]==0xE5) {
		return retfile;
	}
	retfile.valid = true;
	retfile.size = dir_entry.FileSize;
	char *ptr = buf;
	for (uint8_t i = 0; i < 8; i++) {
		if (dir_entry.Filename[i]!=' ')
			*ptr++ = dir_entry.Filename[i];
		if (i==7&&dir_entry.Extension[0]!=' ')
			*ptr++ = '.';
	}
	for (uint8_t i = 0; i < 3; i++) {
		if (dir_entry.Extension[i]!=' ')
			*ptr++ = dir_entry.Extension[i];
		if (dir_entry.FileAttributes&0x10&&i==2)
			*ptr++ = '/';
	}
	return retfile;
}