#include <kernel/idt.h>

#define KEY_PRESSED 0
#define KEY_RELEASED 128

#define LSHIFT 42
#define RSHIFT 54
#define CTRL 29
#define ALT 56
#define CAPSLCK 58
#define SCRLCK 70
#define NUMLCK 69

char kbdus[] = {
	0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,
	'\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',
	'\n', 0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
	0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
	'*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '7', '8', '9',
	'-', '4', '5', '6', '+', '1', '2', '3', '0', '.'
};
char kbdus_shift[] = {
	0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0,
	'\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}',
	'\n', 0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~',
	0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
	'*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '7', '8', '9',
	'-', '4', '5', '6', '+', '1', '2', '3', '0', '.'
};
char kbdus_caps[] = {
	0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,
	'\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']',
	'\n', 0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'', '`',
	0, '\\', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',', '.', '/', 0,
	'*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '7', '8', '9',
	'-', '4', '5', '6', '+', '1', '2', '3', '0', '.'
};

uint8_t last_scancode;
char lastkey_char;
_Bool key_read;
_Bool special_read;
_Bool ctrl = false;
_Bool shift = false;
_Bool alt = false;
_Bool numlck = false;
_Bool scrlck = false;
_Bool capslck = false;

void init_kbd() {
	
}

int getkey() {
	if (!special_read) {
		special_read = true;
		return last_scancode;
	} else {
		return 0;
	}
}

int get_raw_scancode() {
	return last_scancode;
}

char getchar() {
	if (!key_read) {
		key_read = true;
		return lastkey_char;
	} else {
		return 0;
	}
}

void kbd_ack(void){
  while(!(inb(0x60)==0xfa));
}

char kbdstatus = 0;

void update_leds(char leds) {
   outb(0x60, 0x0ed);
   outb(0x60, leds&0x07);
}

void toggle_numlck()
{
  numlck = !numlck;
  kbdstatus ^= 2;
  update_leds(kbdstatus);
}

void toggle_scrlck()
{
  scrlck = !scrlck;
  kbdstatus ^= 1;
  update_leds(kbdstatus);
}

void toggle_capslck()
{
  capslck = !capslck;
  kbdstatus ^= 4;
  update_leds(kbdstatus);
}

void kbd_handler() {
	last_scancode = inb(0x60);
	if (last_scancode>128) {
		if (last_scancode==LSHIFT+KEY_RELEASED||last_scancode==RSHIFT+KEY_RELEASED) {
			shift = false;
		} else if (last_scancode==CTRL+KEY_RELEASED) {
			ctrl = false;
		} else if (last_scancode==ALT+KEY_RELEASED) {
			alt = false;
		} else if (last_scancode==CAPSLCK+KEY_RELEASED) {
			toggle_capslck();
		} else if (last_scancode==SCRLCK+KEY_RELEASED) {
			toggle_scrlck();
		} else if (last_scancode==NUMLCK+KEY_RELEASED) {
			toggle_numlck();
		}
	} else {
		key_read = false;
		special_read = false;
		lastkey_char=0;
		if (last_scancode==LSHIFT+KEY_PRESSED||last_scancode==RSHIFT+KEY_PRESSED) {
			shift = true;
		} else if (last_scancode==CTRL+KEY_PRESSED) {
			ctrl = true;
		} else if (last_scancode==ALT+KEY_PRESSED) {
			alt = true;
		} else {
			lastkey_char = (shift) ? kbdus_shift[last_scancode] : ((capslck) ? kbdus_caps[last_scancode] : kbdus[last_scancode]);
		}
	}
}
