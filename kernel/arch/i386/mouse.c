#include <kernel/kbd.h>

uint8_t mouse_cycle = 0;
int8_t pkt[3];
uint32_t max_width = 1024;
uint32_t max_height = 768;
uint32_t x = ((uint32_t)-1)/4;
uint32_t y = ((uint32_t)-1)/4;
uint8_t buttons = 0;

void mouse_set_resolution(uint32_t width, uint32_t height) {
	max_width = width;
	x=0;
	max_height = height;
	y=0;
}

uint32_t mouse_getx() {
	return x;
}

uint32_t mouse_gety() {
	return y;
}

uint8_t mouse_getbuttons() {
	return buttons;
}

void mouse_set_override(uint32_t _x, uint32_t _y) {
	x = _x;
	y = _y;
}

void mouse_buttons_override(uint8_t _buttons) {
	buttons = _buttons;
	printf("Mouse X: %d, Mouse Y: %d, Buttons: %#\n",x,y,(uint64_t)buttons);
}

//Mouse functions
void mouse_handler()
{
	switch(mouse_cycle)
	{
		case 0:
			pkt[0] = inb(0x60);
			mouse_cycle++;
			break;
		case 1:
			pkt[1] = inb(0x60);
			mouse_cycle++;
			break;
		case 2:
			pkt[2] = inb(0x60);
			if (-pkt[0]>(int64_t)x)
				x = 0;
			else if (pkt[0]+x>max_width-1)
				x = max_width-1;
			else
				x += pkt[0];
			pkt[1] = -pkt[1];
			if (-pkt[1]>(int64_t)y)
				y = 0;
			else if (pkt[1]+y>max_height-1)
				y = max_height-1;
			else
				y += pkt[1];
			buttons = pkt[2];
			mouse_cycle=0;
			//printf("X: %d Y: %d B: %d\n",(uint32_t)x,(uint32_t)y,(uint32_t)buttons);
			break;
	}
}

inline void mouse_wait(uint8_t bit) {
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
	mouse_wait(1);
	outb(0x64,0xD4);
	mouse_wait(1);
	outb(0x60,0xF6);
	mouse_wait(0);
	inb(0x60);
	mouse_wait(1);
	outb(0x64,0xD4);
	mouse_wait(1);
	outb(0x60,0xF4);
	mouse_wait(0);
	inb(0x60);
}