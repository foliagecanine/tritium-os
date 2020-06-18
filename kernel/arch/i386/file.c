#include <fs/file.h>

#define SUCCESS 					0
#define INCORRECT_FS_TYPE 1
#define DRIVE_IN_USE 			2
#define UNKNOWN_ERROR		3

FSMOUNT *mounts;
uint32_t numActiveMounts = 0;

void init_file() {
	mounts = alloc_page(1);
	memset(mounts,0,4096);
}

uint8_t unmountDrive(uint8_t drive) {
	if (mounts[drive].mountEnabled) {
		numActiveMounts--;
		free_page(mounts[drive].mount,1);
	}
	FSMOUNT newMount;
	memset(&newMount,0,sizeof(FSMOUNT));
	newMount.mountEnabled = false;
	mounts[numActiveMounts] = newMount;
	return 0;
}

uint8_t mountDrive(uint8_t drive) {
	if (detect_fat12(drive)) {
		FSMOUNT newMount = MountFAT12(drive);
		if (strcmp(newMount.type,"FAT12")) {
			mounts[numActiveMounts] = newMount;
			numActiveMounts++;
			return SUCCESS;
		} else {
			return UNKNOWN_ERROR;
		}
	} else if (detect_fat16(drive)) {
		FSMOUNT newMount = MountFAT16(drive);
		if (strcmp(newMount.type,"FAT16")) {
			mounts[numActiveMounts] = newMount;
			numActiveMounts++;
			return SUCCESS;
		} else {
			return UNKNOWN_ERROR;
		}
	} else {
		return INCORRECT_FS_TYPE;
	}
}

//Similar to brokenthorn (see urls in fat12.c)
#pragma GCC diagnostic ignored "-Wunused-parameter"
FILE fopen(const char *filename, const char *mode) {
	//Check if filename is present (we can't open nothing)
	if (filename) {
		uint8_t device;
		//Validate and convert to drive number
		if (filename[1]==':'&&filename[2]=='/') {
			device = tolower(filename[0])-'a';
			//Filename without drive prefix
			char* flongname = (char*) filename+2;
			if (flongname&&device<9) {
				if (mounts[device].mountEnabled) {
					if (strcmp(mounts[device].type,"FAT12")&&detect_fat12(mounts[device].drive)) {
						FAT12_MOUNT *mnt = (FAT12_MOUNT *)mounts[device].mount;
						uint32_t RDO = ((mnt->RootDirectoryOffset)*512+1);
						uint32_t NRDE = mnt->NumRootDirectoryEntries;
						FILE retFile = FAT12_fopen(RDO, NRDE, flongname,mounts[device].drive,*mnt,0);
						retFile.mountNumber = device;
						return retFile;
					} else if (strcmp(mounts[device].type,"FAT16")&&detect_fat16(mounts[device].drive)) {
						FAT16_MOUNT *mnt = (FAT16_MOUNT *)mounts[device].mount;
						uint32_t RDO = ((mnt->RootDirectoryOffset)*512+1);
						uint32_t NRDE = mnt->NumRootDirectoryEntries;
						FILE retFile = FAT16_fopen(RDO, NRDE, flongname,mounts[device].drive,*mnt,0);
						retFile.mountNumber = device;
						return retFile;
					}
				}
			}
		}
	}
	//If we receive an error then just return an invalid file
	FILE noFile;
	noFile.valid = false;
	return noFile;
}

uint8_t fread(FILE *file, char *buf, uint64_t start, uint64_t len) {
	if (!file)
		return 1;
	if (strcmp(mounts[file->mountNumber].type,"FAT12")) {
		return FAT12_fread(file,buf,(uint32_t)start,(uint32_t)len,mounts[file->mountNumber].drive);
	} else if (strcmp(mounts[file->mountNumber].type,"FAT16")) {
		return FAT16_fread(file,buf,(uint32_t)start,(uint32_t)len,mounts[file->mountNumber].drive);
	}
	return 1;
}

//We expect buf to be 256 characters long
FILE readdir(FILE *file, char* buf, uint32_t n) {
	FILE retfile;
	memset(&retfile,0,sizeof(FILE));
	if (!file)
		return retfile;
	if (strcmp(mounts[file->mountNumber].type,"FAT12")) {
		//return FAT12_readdir(file,buf,n,mounts[file->mountNumber].drive);
	} else if (strcmp(mounts[file->mountNumber].type,"FAT16")) {
		return FAT16_readdir(file,buf,n,mounts[file->mountNumber].drive);
	}
	return retfile;
}

FSMOUNT getDiskMount(uint8_t drive) {
	return mounts[drive];
}