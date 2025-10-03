#include <kernel/kbd.h>
#include <kernel/sysfunc.h>
#include <stdbool.h>
#include <stdint.h>

#define PS2_OUT_PORT 0x60
#define PS2_IN_PORT 0x60

#define PS2_KEY_PRESSED 0
#define PS2_KEY_RELEASED 128

#define PS2_LSHIFT 42
#define PS2_RSHIFT 54
#define PS2_CTRL 29
#define PS2_ALT 56
#define PS2_CAPSLCK 58
#define PS2_SCRLCK 70
#define PS2_NUMLCK 69

#define PS2_LED_UPDATE 0xED
#define PS2_LED_SCROLL 0x01
#define PS2_LED_NUM 0x02
#define PS2_LED_CAPS 0x04

#define PS2_ACK 0xFA

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
bool key_read;
bool special_read;
bool ctrl = false;
bool shift = false;
bool alt = false;
bool numlck = false;
bool scrlck = false;
bool capslck = false;

uint32_t get_kbddata() {
	if (!special_read) {
		special_read = true;
		return (((ctrl&1) | ((shift&1)<<1) | ((alt&1)<<2) | ((numlck&1)<<3) | ((scrlck&1)<<4) | ((capslck&1)<<5) | (3<<6))<<16) | ((lastkey_char)<<8) | last_scancode;
	} else {
		return (((ctrl&1) | ((shift&1)<<1) | ((alt&1)<<2) | ((numlck&1)<<3) | ((scrlck&1)<<4) | ((capslck&1)<<5) | (3<<6))<<16) | 0;
	}
}

unsigned int getkey_a() {
	if (!special_read) {
		special_read = true;
		return last_scancode;
	} else {
		return 0;
	}
}

unsigned int getkey() {
	if (!special_read) {
		special_read = true;
		if (last_scancode > PS2_KEY_PRESSED)
		{
			return last_scancode - PS2_KEY_PRESSED;
		}
		else
		{
			return last_scancode;
		}
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

void print_keys() {
	char c = getchar();
	if (c) {
		printf("%c", c);
	} else if (getkey() == 14) {
		terminal_backup();
		putchar(' ');
		terminal_backup();
	}
 }
 
 char scancode_to_char(unsigned int scancode) {
	return (shift) ? kbdus_shift[scancode] : ((capslck) ? kbdus_caps[scancode] : kbdus[scancode]);
 }

void kbd_ack(void){
  while(!(inb(PS2_IN_PORT) == PS2_ACK));
}

char kbdstatus = 0;

void update_leds(char leds) {
   outb(PS2_OUT_PORT, PS2_LED_UPDATE);
   outb(PS2_OUT_PORT, leds & (PS2_LED_SCROLL | PS2_LED_NUM | PS2_LED_CAPS));
}

void toggle_numlck()
{
  numlck = !numlck;
  kbdstatus ^= PS2_LED_NUM;
  update_leds(kbdstatus);
}

void toggle_scrlck()
{
  scrlck = !scrlck;
  kbdstatus ^= PS2_LED_SCROLL;
  update_leds(kbdstatus);
}

void toggle_capslck()
{
  capslck = !capslck;
  kbdstatus ^= PS2_LED_CAPS;
  update_leds(kbdstatus);
}

void decode_scancode() {
	if (last_scancode > PS2_KEY_RELEASED) {
		uint8_t released_key = last_scancode - PS2_KEY_RELEASED;

		if (released_key == PS2_LSHIFT || released_key == PS2_RSHIFT) {
			shift = false;
		} else if (released_key == PS2_CTRL) {
			ctrl = false;
		} else if (released_key == PS2_ALT) {
			alt = false;
		} else if (released_key == PS2_CAPSLCK) {
			toggle_capslck();
		} else if (released_key == PS2_SCRLCK) {
			toggle_scrlck();
		} else if (released_key == PS2_NUMLCK) {
			toggle_numlck();
		}

		special_read = false;
	} else {
		uint8_t pressed_key = last_scancode - PS2_KEY_PRESSED;

		key_read = false;
		special_read = false;
		lastkey_char = 0;
		if (pressed_key == PS2_LSHIFT ||
			pressed_key == PS2_RSHIFT) {
			shift = true;
		} else if (pressed_key == PS2_CTRL) {
			ctrl = true;
		} else if (pressed_key == PS2_ALT) {
			alt = true;
		} else {
			lastkey_char = (shift) ? kbdus_shift[pressed_key] : ((capslck) ? kbdus_caps[pressed_key] : kbdus[pressed_key]);
		}
	}
}

void kbd_handler() {
	last_scancode = inb(PS2_IN_PORT);
	decode_scancode();
}

void insert_scancode(uint8_t scancode) {
	last_scancode = scancode;
	dprintf("Scancode: %X\n",(uint64_t)scancode);
	decode_scancode();
}