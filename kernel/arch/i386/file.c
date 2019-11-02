#include <fs/file.h>

#define SUCCESS 					0
#define INCORRECT_FS_TYPE 1
#define DRIVE_IN_USE 			2
#define UNKNOWN_ERROR		3

//One per disk for now
FSMOUNT mounts[8];

uint8_t unmountDrive(uint8_t drive) {
	if (mounts[drive].mountEnabled) {
		free(mounts[drive].mount);
	}
	FSMOUNT newMount;
	memset(&newMount,0,sizeof(FSMOUNT));
	newMount.mountEnabled = false;
	mounts[drive] = newMount;
	return 0;
}

uint8_t mountDrive(uint8_t drive) {
	if (detect_fat12(drive)) {
		FSMOUNT newMount = MountFAT12(drive);
		if (strcmp(newMount.type,"FAT12")) {
			mounts[drive] = newMount;
			return SUCCESS;
		} else {
			return UNKNOWN_ERROR;
		}
	} else {
		return INCORRECT_FS_TYPE;
	}
}

//Similar to brokenthorn (see urls in fat12.c)
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
				if (mounts[device].mountEnabled&&strcmp(mounts[device].type,"FAT12")&&detect_fat12(device)) {
					FAT12_MOUNT *mnt = (FAT12_MOUNT *)mounts[device].mount;
					uint32_t RDO = ((mnt->RootDirectoryOffset)*512+1);
					uint32_t NRDE = mnt->NumRootDirectoryEntries;
					FILE retFile = FAT12_fopen(RDO, NRDE, flongname,device,*mnt,0);
					retFile.mountNumber = device;
					return retFile;
				}
			}
		} else {
			
		}
	}
	//If we receive an error then just return an invalid file
	FILE noFile;
	noFile.valid = false;
	return noFile;
}

void fread(FILE *file, char *buf, uint64_t start, uint64_t len) {
	if (!file)
		return;
	if (strcmp(mounts[file->mountNumber].type,"FAT12")) {
		FAT12_fread(file,buf,(uint32_t)start,(uint32_t)len,mounts[file->mountNumber].drive);
	}
}

FSMOUNT getDiskMount(uint8_t drive) {
	return mounts[drive];
}