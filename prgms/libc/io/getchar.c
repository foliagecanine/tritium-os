#include <stdio.h>

char savechar;
unsigned int savekey;
bool charused = true;
bool keyused = true;

char getchar() {
	if (charused) {
		uint32_t out;
		asm volatile ("mov $5,%%eax; int $0x80; mov %%eax, %0":"=r"(out));
		savekey = (unsigned int)(out&0xFF);
		keyused = false;
		charused = true;
		if (savekey<0x80)
			return (char)((out>>8)&0xFF);
		else
			return 0;
	} else {
		charused = true;
		return savechar;
	}
}

unsigned int getkey() {
	if (keyused) {
		uint32_t out;
		asm volatile ("mov $5,%%eax; int $0x80; mov %%eax, %0":"=r"(out));
		if ((out&0xFF)<0x80) {
			savechar = (unsigned int)((out>>8)&0xFF);
			charused = false;
		}
		keyused = true;
		return (unsigned int)(out&0xFF);
	} else {
		keyused = true;
		return savekey;
	}
}