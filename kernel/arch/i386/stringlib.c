#include <kernel/easylib.h>

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