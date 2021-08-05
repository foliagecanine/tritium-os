#include <fs/fat.h>

int findCharInArray(char *array, char c) {
	for (uint8_t i = 0; i < strlen(array); i++) {
		if (array[i] == c) {
			return i;
		}
	}
	return -1;
}

char intToChar(uint8_t num) {
	const char nums[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
	if (num < 10) {
		return nums[num];
	} else {
		return 0;
	}
}

/*
 * This function should probably be optimized!
 */
void LongToShortFilename(char *longfn, char *shortfn) {
	// Longfilename.extension -> LONGFI~6EXT, textfile.txt -> TEXTFILETXT, short.txt -> SHORT   TXT
	memset(shortfn, ' ', 11); // Fill with spaces

	// First check for . and ..
	if (strcmp(longfn, ".")) {
		strcpy(shortfn, ".          ");
		return;
	}
	if (strcmp(longfn, "..")) {
		strcpy(shortfn, "..         ");
		return;
	}

	// Next check if it is already a short filename
	if (!strchr(longfn, '.') && strlen(longfn) == 11) {
		strcpy(shortfn, longfn);
		return;
	}

	// Then do the rest
	int locOfDot = findCharInArray(longfn, '.');
	if (locOfDot > 8) {
		memcpy(shortfn, longfn, 6);
		shortfn[6] = '~';
		if ((locOfDot - 6) > 9) {
			shortfn[7] = '~';
		} else {
			shortfn[7] = intToChar(locOfDot - 6);
		}
	} else {
		if (locOfDot != -1) {
			memcpy(shortfn, longfn, locOfDot);
		} else if (strlen(longfn) < 9) { // If there is no dot then just copy the whole thing (up to 8).
			memcpy(shortfn, longfn, strlen(longfn));
		} else {
			memcpy(shortfn, longfn, 6);
			shortfn[6] = '~';
			if ((strlen(longfn) - 6) > 9) {
				shortfn[7] = '~';
			} else {
				shortfn[7] = intToChar(strlen(longfn) - 6);
			}
		}
	}

	// Check for extension
	if (locOfDot != -1) {
		// Yes extension. Copy up to the first 3 letters. If more than 3 do this: extens -> e~5
		int extLen = strlen(longfn) - locOfDot - 1;

		if (extLen > 0)
			shortfn[8] = longfn[locOfDot + 1];

		if (extLen > 1 && extLen < 4)
			shortfn[9] = longfn[locOfDot + 2];

		if (extLen > 2 && extLen < 4)
			shortfn[10] = longfn[locOfDot + 3];

		if (extLen >= 4) {
			shortfn[9] = '~';
			if ((extLen - 1) > 9) {
				shortfn[10] = '~';
			} else {
				shortfn[10] = intToChar(extLen - 1);
			}
		}

		shortfn[11] = 0; // End string
	} else {
		// No extension. Just put 3 spaces.
		shortfn[8] = ' ';
		shortfn[9] = ' ';
		shortfn[10] = ' ';
		shortfn[11] = 0; // End string
	}

	// Uppercase our name. 8.3 only stores uppercase (not counting LFN; not going to do LFN for a while)
	for (uint8_t i = 0; i < 12; i++) {
		shortfn[i] = toupper(shortfn[i]);
	}

	// Add any neccesary padding
	/* if (shortfn[0]!=0) {
	  for (uint8_t i = 0; i<11; i++) {
	    if (shortfn[i]==0)
	      shortfn[i] = ' ';
	  }
	} */
}