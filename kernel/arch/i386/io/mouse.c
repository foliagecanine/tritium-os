#include <kernel/kbd.h>

uint8_t mouse_cycle = 0;
int8_t pkt[3];

int mouse_delta_x;
int mouse_delta_y;
uint8_t buttons;

//Mouse functions
void mouse_handler()
{
	switch(mouse_cycle)
	{
		case 0:
			pkt[0] = inb(0x60);
			if ((pkt[0] & 0b00001000) == 0) // Occasionally the packets get out of order. Make sure packet 0 is valid.
				break;
			mouse_cycle++;
			break;
		case 1:
			pkt[1] = inb(0x60);
			mouse_cycle++;
			break;
		case 2:
			pkt[2] = inb(0x60);
			mouse_cycle=0;

			if (!(pkt[0] & 0xC0)) {
				mouse_delta_x += pkt[1];
				mouse_delta_y -= pkt[2];
			}

			buttons = pkt[0] & 0b00000111; // Just left, right, and middle click

			break;
	}
}

void mouse_wait(uint8_t bit) {
	uint16_t t=10000;
	if(bit==0) {
		while(t--) //Data
			if(inb(0x64)&1)
				return;
	} else {
		while(t--)
			if(!(inb(0x64)&2))
				return;
	}
}

void mouse_send_command(uint8_t cmd) {
	mouse_wait(1);
	outb(0x64,0xD4);
	mouse_wait(1);
	outb(0x60,cmd);
}

uint8_t mouse_get_ack() {
	mouse_wait(0);
	return inb(0x60);
}

void init_mouse() {
	mouse_wait(1);
	outb(0x64, 0xA8);
	mouse_wait(1);
	outb(0x64, 0x20);
	mouse_wait(0);
	uint8_t tmp = inb(0x60) | 2;
	mouse_wait(1);
	outb(0x64, 0x60);
	mouse_wait(1);
	outb(0x60, tmp);
	mouse_send_command(0xF6);
	mouse_get_ack();
	mouse_send_command(0xF4);
	mouse_get_ack();
}

void mouse_add_delta(int x, int y) {
	mouse_delta_x += x;
	mouse_delta_y += y;
}

void mouse_buttons_override(uint8_t new_buttons) {
	buttons = new_buttons;
}

int get_mousedataX() {
	int retval = mouse_delta_x;
	mouse_delta_x = 0;
	return retval;
}

int get_mousedataY() {
	int retval = mouse_delta_y;
	mouse_delta_y = 0;
	return retval;
}

int get_mousedataZ() { // Scroll
	return 0; // We haven't enabled scrolling
}

uint32_t get_mousedataB() { // Buttons
	return buttons;
}
