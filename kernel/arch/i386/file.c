#include <fs/file.h>

#define SUCCESS 					0
#define INCORRECT_FS_TYPE 1
#define DRIVE_IN_USE 			2

//One per disk for now
FSMOUNT mounts[8];

uint8_t unmountDrive(uint8_t drive) {
	if (mounts[drive].mountEnabled) {
		free(mounts[drive].mount);
	}
	FSMOUNT newMount;
	mounts[drive] = newMount;
}

uint8_t mountAsFAT12(uint8_t drive) {
	if (detect_fat12(drive)) {
		mounts[drive] = MountFAT12(drive);
		return SUCCESS;
	} else {
		return INCORRECT_FS_TYPE;
	}
}