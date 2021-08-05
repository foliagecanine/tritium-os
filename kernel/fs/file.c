#include <fs/file.h>

// Codes:
// 0 = Success
// 1 = Not found
// 2 = Directory
// 3 = Invalid Filename
// 4 = Unknown Error
// 5 = Invalid Filesystem
// 8+ = Drive error = (error_code - 8) (see ahci.c, ide.c)

FSMOUNT *mounts;
uint32_t num_active_mounts = 0;

void init_file() {
	mounts = alloc_page(1);
	memset(mounts, 0, 4096);
}

uint8_t unmount_drive(uint32_t mount) {
	if (mounts[mount].mountEnabled) {
		num_active_mounts--;
		free_page(mounts[mount].mount, 1);
	}
	FSMOUNT newMount;
	memset(&newMount, 0, sizeof(FSMOUNT));
	newMount.mountEnabled = false;
	mounts[num_active_mounts] = newMount;
	return SUCCESS;
}

uint8_t mount_drive(uint32_t drive_num) {
	if (detect_fat12(drive_num)) {
		FSMOUNT new_mount = MountFAT12(drive_num);
		if (strcmp(new_mount.type, "FAT12")) {
			mounts[num_active_mounts] = new_mount;
			num_active_mounts++;
			return SUCCESS;
		} else {
			return UNKNOWN_ERROR;
		}
	} else if (detect_fat16(drive_num)) {
		FSMOUNT new_mount = MountFAT16(drive_num);
		if (strcmp(new_mount.type, "FAT16")) {
			mounts[num_active_mounts] = new_mount;
			num_active_mounts++;
			return SUCCESS;
		} else {
			return UNKNOWN_ERROR;
		}
	} else {
		return INCORRECT_FS_TYPE;
	}
}

// Similar to brokenthorn (see urls in fat12.c)
FILE fopen(const char *filename, const char *mode) {
	(void)mode;
	// Check if filename is present (we can't open nothing)
	if (filename) {
		uint8_t device;
		// Validate and convert to drive number
		if (filename[1] == ':' && filename[2] == '/') {
			device = tolower(filename[0]) - 'a';
			// Filename without drive prefix
			char *flongname = (char *)filename + 2;
			if (flongname && device < 9) {
				if (mounts[device].mountEnabled) {
					if (strcmp(mounts[device].type, "FAT12") && detect_fat12(mounts[device].drive)) {
						FAT12_MOUNT *mnt = (FAT12_MOUNT *)mounts[device].mount;
						uint32_t     RDO = ((mnt->RootDirectoryOffset) * 512 + 1);
						uint32_t     NRDE = mnt->NumRootDirectoryEntries;
						FILE         retFile = FAT12_fopen(RDO, NRDE, flongname, mounts[device].drive, *mnt, 0);
						retFile.mountNumber = device;
						return retFile;
					} else if (strcmp(mounts[device].type, "FAT16") && detect_fat16(mounts[device].drive)) {
						FAT16_MOUNT *mnt = (FAT16_MOUNT *)mounts[device].mount;
						uint32_t     RDO = ((mnt->RootDirectoryOffset) * 512 + 1);
						uint32_t     NRDE = mnt->NumRootDirectoryEntries;
						FILE         retFile = FAT16_fopen(RDO, NRDE, flongname, mounts[device].drive, *mnt, 0);
						retFile.mountNumber = device;
						return retFile;
					}
				}
			}
		}
	}
	// If we receive an error then just return an invalid file
	FILE noFile;
	noFile.valid = false;
	return noFile;
}

uint8_t fread(FILE *file, char *buf, uint64_t start, uint64_t len) {
	if (!file)
		return UNKNOWN_ERROR;
	if (strcmp(mounts[file->mountNumber].type, "FAT12")) {
		return FAT12_fread(file, buf, (uint32_t)start, (uint32_t)len, mounts[file->mountNumber].drive);
	} else if (strcmp(mounts[file->mountNumber].type, "FAT16")) {
		return FAT16_fread(file, buf, (uint32_t)start, (uint32_t)len, mounts[file->mountNumber].drive);
	}
	return INCORRECT_FS_TYPE;
}

uint8_t fwrite(FILE *file, char *buf, uint64_t start, uint64_t len) {
	if (!file)
		return UNKNOWN_ERROR;
	if (strcmp(mounts[file->mountNumber].type, "FAT12")) {
		return INCORRECT_FS_TYPE;
	} else if (strcmp(mounts[file->mountNumber].type, "FAT16")) {
		return FAT16_fwrite(file, buf, (uint32_t)start, (uint32_t)len, mounts[file->mountNumber].drive);
	}
	return INCORRECT_FS_TYPE;
}

// We expect buf to be 256 characters long
FILE readdir(FILE *file, char *buf, uint32_t n) {
	FILE retfile;
	memset(&retfile, 0, sizeof(FILE));
	if (!file)
		return retfile;
	if (strcmp(mounts[file->mountNumber].type, "FAT12")) {
		return FAT12_readdir(file, buf, n, mounts[file->mountNumber].drive);
	} else if (strcmp(mounts[file->mountNumber].type, "FAT16")) {
		return FAT16_readdir(file, buf, n, mounts[file->mountNumber].drive);
	}
	return retfile;
}

FILE fcreate(char *filename) {
	FILE retfile;
	memset(&retfile, 0, sizeof(FILE));
	FILE f = fopen(filename, "r");
	if (f.valid)
		return retfile;
	if (filename) {
		uint8_t device;
		// Validate and convert to drive number
		if (filename[1] == ':' && filename[2] == '/') {
			device = tolower(filename[0]) - 'a';
			// Filename without drive prefix
			char *flongname = (char *)filename + 2;
			if (flongname && device < 9) {
				if (mounts[device].mountEnabled) {
					if (strcmp(mounts[device].type, "FAT12") && detect_fat12(mounts[device].drive)) {
						// Notimplemented
					} else if (strcmp(mounts[device].type, "FAT16") && detect_fat16(mounts[device].drive)) {
						retfile = FAT16_fcreate(filename, *((FAT16_MOUNT *)mounts[device].mount), mounts[device].drive);
					}
				}
			}
		}
	}
	return retfile;
}

uint8_t fdelete(char *filename) {
	FILE f = fopen(filename, "w");
	if (!f.valid) {
		return FILE_NOT_FOUND;
	}
	if (f.directory) {
		return IS_DIRECTORY;
	}
	if (filename) {
		uint8_t device;
		// Validate and convert to drive number
		if (filename[1] == ':' && filename[2] == '/') {
			device = tolower(filename[0]) - 'a';
			// Filename without drive prefix
			char *flongname = (char *)filename + 2;
			if (flongname && device < 9) {
				if (mounts[device].mountEnabled) {
					if (strcmp(mounts[device].type, "FAT12") && detect_fat12(mounts[device].drive)) {
						// Notimplemented
					} else if (strcmp(mounts[device].type, "FAT16") && detect_fat16(mounts[device].drive)) {
						return FAT16_fdelete(filename, mounts[device].drive);
					}
				}
			}
		}
	}
	return UNKNOWN_ERROR;
}

uint8_t ferase(char *filename) {
	FILE f = fopen(filename, "w");
	if (!f.valid) {
		return FILE_NOT_FOUND;
	}
	if (f.directory) {
		return IS_DIRECTORY;
	}
	if (filename) {
		uint8_t device;
		// Validate and convert to drive number
		if (filename[1] == ':' && filename[2] == '/') {
			device = tolower(filename[0]) - 'a';
			// Filename without drive prefix
			char *flongname = (char *)filename + 2;
			if (flongname && device < 9) {
				if (mounts[device].mountEnabled) {
					if (strcmp(mounts[device].type, "FAT12") && detect_fat12(mounts[device].drive)) {
						// Notimplemented
					} else if (strcmp(mounts[device].type, "FAT16") && detect_fat16(mounts[device].drive)) {
						return FAT16_ferase(filename, *((FAT16_MOUNT *)mounts[device].mount), mounts[device].drive);
					}
				}
			}
		}
	}
	return UNKNOWN_ERROR;
}

FSMOUNT get_disk_mount(uint32_t drive) { return mounts[drive]; }
