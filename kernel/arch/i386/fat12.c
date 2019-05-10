#include <kernel/fs.h>

//Thanks to https://www.win.tue.nl/~aeb/linux/fs/fat/fat-1.html for the fat12 details
//along with http://www.brokenthorn.com/Resources/OSDev22.html for when I get stuck
//However, I wrote this wholly myself

typedef struct {
	uint8_t OEMName[8];
	uint16_t BytesPerSector;
	uint8_t SectorsPerCluster;
	uint8_t NumFATs;
	uint16_t NumRootDirectoryEntries;
	uint16_t NumTotalSectors;
	uint8_t MediaDescriptorType;
	uint16_t NumSectorsPerFAT;
	uint16_t NumSectorsPerTrack;
	uint16_t NumHeads;
	uint16_t NumHiddenSectors;
} BPB, *PBPB; //Hehe: ptr BIOS parameter block

typedef struct {
	uint8_t StartCode[3];
	BPB BiosParameterBlock;
	uint8_t Bootstrap[480]; //Funny thing is brokenthorn has anther BPB and reduces the size of the bootstrap... No clue, but I'll follow the win.tue.nl one.
} BOOTSECT, *PBOOTSECT;

