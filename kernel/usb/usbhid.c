#include <usb/hid.h>

uint8_t calc_interval(uint8_t interval_in) {
	for (uint8_t i = 0; i < 8; i++) {
		if (interval_in & (0x80 >> i))
			return 7 - i;
	}
	return 7;
}

const uint8_t usb_scancode_to_ps2[256] = {
    0,    0,    0,    0,    0x1E, 0x30, 0x2E, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 0x25, 0x26, 0x32, 0x31, 0x18, 0x19, 0x10, 0x13, 0x1F, 0x14, 0x16, 0x2F, 0x11, 0x2D, 0x15, 0x2C, // NULL,NULL,NULL,"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,                                                                                                                         // "1234567890"
    0x1C, 0x01, 0x0E, 0x0F, 0x39, 0x0C, 0x0D, 0x1A, 0x1B, 0x2B, 0,    0x27, 0x28, 0x29, 0x33, 0x34, 0x35, 0x3A,                                                                         // ENTER,ESC,BACKSPACE,TAB,SPACE,"-=[]\;'`,./",CAPSLOCK
    0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x57, 0x58,                                                                                                             // F1,F2,F3,F4,F5,F6,F7,F8,F9,F10,F11,F12
    0x54, 0x46, 0,    0,    0x47, 0x49, 0x53, 0x4F, 0x51, 0x4D, 0x4B, 0x50, 0x48,                                                                                                       // SYSRQ,SCROLLLOCK,PAUSE,INSERT,HOME,PAGEUP,DELETE,END,PAGEDOWN,RIGHT,LEFT,DOWN,UP
    0x45, 0,    0,    0,    0,    0x1C, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0                                                                                   // NUMLOCK,KPSLASH,KPASTERISK,KPMINUS,KPENTER,KP"1234567890",KPDECIMAL
};
uint8_t     keyboard_buffer_cmp[8];
uint64_t    repeat_key_press_time = 0;
uint8_t     keyboard_repeat_key = 0;
uint16_t    usb_keyboard_repeat_initial_delay = 500; // 500 ms
uint16_t    usb_keyboard_repeat_repeat_delay = 25;   // 25 ms
usb_device *dev_kbd = 0;

bool key_exists(uint8_t key, uint8_t *keylist) {
	for (uint8_t i = 0; i < 6; i++) {
		if (key == keylist[i])
			return true;
	}
	return false;
}

void insert_repeatable(uint8_t scancode) {
	dprintf("Make : %#\n", (uint64_t)scancode);
	insert_scancode(usb_scancode_to_ps2[scancode]);
	keyboard_repeat_key = scancode;
	repeat_key_press_time = get_ticks();
}

void remove_repeatable(uint8_t scancode) {
	dprintf("Break: %#\n", (uint64_t)scancode);
	if (keyboard_repeat_key == scancode)
		keyboard_repeat_key = 0;
	insert_scancode(usb_scancode_to_ps2[scancode] + PS2_KEY_RELEASED);
}

uint8_t PS2_modifiers[8] = {PS2_CTRL, PS2_LSHIFT, PS2_ALT, 0, PS2_CTRL, PS2_RSHIFT, PS2_ALT, 0};

void check_modifiers(uint8_t key, uint8_t bufferkey, uint8_t direction) {
	for (uint8_t i = 0; i < 8; i++) {
		uint8_t keymod = key & (1 << i);
		uint8_t bkeymod = bufferkey & (1 << i);
		if (!direction) {
			if (keymod && !bkeymod) {
				insert_scancode(PS2_modifiers[i]);
			}
		} else {
			if (!keymod && bkeymod) {
				insert_scancode(PS2_modifiers[i] + PS2_KEY_RELEASED);
			}
		}
	}
}

void hid_kbd_irq(uint16_t dev_addr) {
	usb_device *device = usb_device_from_addr(dev_addr);
	if (usb_refresh_interval(dev_addr, device->driver0)) {
		uint8_t *kbd_input = device->driver1;

		// Check modifiers
		check_modifiers(kbd_input[0], keyboard_buffer_cmp[0], 0);
		check_modifiers(kbd_input[0], keyboard_buffer_cmp[0], 1);

		// Check for newly pressed keys
		for (uint8_t i = 2; i < 8; i++) {
			if (!key_exists(kbd_input[i], keyboard_buffer_cmp + 2))
				insert_repeatable(kbd_input[i]);
			if (!key_exists(keyboard_buffer_cmp[i], kbd_input + 2))
				remove_repeatable(keyboard_buffer_cmp[i]);
		}

		// Sync the buffer
		memcpy(keyboard_buffer_cmp, kbd_input, 8);
	}
}

void usb_keyboard_repeat() {
	usb_device *tmp_kbd = dev_kbd;
	while (tmp_kbd) {
		uint16_t dev_addr = usb_dev_addr(tmp_kbd->ctrlr_type, tmp_kbd->ctrlrID, tmp_kbd->address);
		hid_kbd_irq(dev_addr);
		tmp_kbd = tmp_kbd->driver2;
	}

	uint64_t ticks = get_ticks();
	if (!(ticks % usb_keyboard_repeat_repeat_delay)) {
		if (keyboard_repeat_key && (ticks - repeat_key_press_time) > usb_keyboard_repeat_initial_delay)
			insert_scancode(usb_scancode_to_ps2[keyboard_repeat_key]);
	}
}

uint8_t keyboard_buffer[8];

bool init_hid_kbd(uint16_t dev_addr, usb_config_desc config, usb_interface_desc interface) {
	// printf("Configuring HID Keyboard\n");

	(void)config; // Remove the "unused variable" warning. The function definitions should match this format whether they use the variables or not.

	// Find endpoint address for the interrupt-in endpoint
	usb_endpoint_desc endpoint;
	uint8_t           endpoint_addr = 0xFF;
	for (uint8_t i = 0; i < interface.num_endpoints; i++) {
		endpoint = usb_get_endpoint_desc(dev_addr, 0, 0, i);
		if (!endpoint.length)
			return false;
		if (endpoint.attributes == 3 && endpoint.endpoint_addr & 0x80) {
			endpoint_addr = endpoint.endpoint_addr & 0xF;
			break;
		}
	}
	if (endpoint_addr == 0xFF)
		return false;
	usb_enable_endpoint(dev_addr, endpoint_addr, USB_ENDPOINT_IN | USB_ENDPOINT_INTERRUPT, calc_interval(endpoint.interval));

	// Use boot protocol
	usb_setup_pkt sp;
	sp.type = 0x21;
	sp.request = 0x0B;
	sp.value = 0;
	sp.index = 0;
	sp.length = 0;
	if (!usb_generic_setup(dev_addr, sp))
		return false;

	// Allocate memory for input
	usb_device *device = usb_device_from_addr(dev_addr);
	device->driver_function = &hid_kbd_irq;
	device->driver1 = keyboard_buffer;
	device->driver0 = usb_create_interval_in(dev_addr, device->driver1, calc_interval(endpoint.interval), endpoint_addr, endpoint.max_pkt_size, 8);
	device->driver2 = dev_kbd;
	dev_kbd = device;

	printf("[HID] Installed HID Keyboard\n");
	return true;
}

void hid_mouse_irq(uint16_t dev_addr) {
	usb_device *device = usb_device_from_addr(dev_addr);
	if (usb_refresh_interval(dev_addr, device->driver0)) {
		int8_t *mouse_input = device->driver1;
		mouse_add_delta(mouse_input[1], mouse_input[2]);
		mouse_buttons_override(*(uint8_t *)device->driver1);
		// printf("%#\n",(uint64_t)mouse_input[3]);
	}
}

uint8_t mouse_buffer[4];

bool init_hid_mouse(uint16_t dev_addr, usb_config_desc config, usb_interface_desc interface) {
	// printf("Configuring HID Mouse\n");

	(void)config; // Remove the "unused variable" warning. The function definitions should match this format whether they use the variables or not.

	// Find endpoint address for the interrupt-in endpoint
	usb_endpoint_desc endpoint;
	uint8_t           endpoint_addr = 0xFF;
	for (uint8_t i = 0; i < interface.num_endpoints; i++) {
		endpoint = usb_get_endpoint_desc(dev_addr, 0, 0, i);
		if (!endpoint.length)
			return false;
		if (endpoint.attributes == 3 && endpoint.endpoint_addr & 0x80) {
			endpoint_addr = endpoint.endpoint_addr & 0xF;
			break;
		}
	}
	if (endpoint_addr == 0xFF)
		return false;
	usb_enable_endpoint(dev_addr, endpoint_addr, USB_ENDPOINT_IN | USB_ENDPOINT_INTERRUPT, calc_interval(endpoint.interval));

	// Use boot protocol
	usb_setup_pkt sp;
	sp.type = 0x21;
	sp.request = 0x0B;
	sp.value = 0;
	sp.index = 0;
	sp.length = 0;
	if (!usb_generic_setup(dev_addr, sp))
		return false;

	// Allocate memory for input
	usb_device *device = usb_device_from_addr(dev_addr);
	device->driver_function = &hid_mouse_irq;
	device->driver1 = mouse_buffer;
	device->driver0 = usb_create_interval_in(dev_addr, device->driver1, calc_interval(endpoint.interval), endpoint_addr, endpoint.max_pkt_size, 4);

	printf("[HID ] Installed HID Mouse\n");
	return true;
}

bool init_hid(uint16_t dev_addr, usb_config_desc config) {
	// printf("Configuring HID device...\n");
	usb_setup_pkt sp;
	sp.type = 0;
	sp.request = 0x09;
	sp.value = config.config_value;
	sp.index = 0;
	sp.length = 0;
	if (!usb_generic_setup(dev_addr, sp))
		return false;
	sp.type = 0x21;
	sp.request = 0x0A;
	sp.value = 0;
	if (!usb_generic_setup(dev_addr, sp))
		return false;
	usb_interface_desc interface = usb_get_interface_desc(dev_addr, 0, 0);
	if (!interface.length)
		return false;

	// printf("Found endpoint!\n");
	if (interface.iprotocol == 1)
		return init_hid_kbd(dev_addr, config, interface);
	else if (interface.iprotocol == 2)
		return init_hid_mouse(dev_addr, config, interface);
	printf("Unknown protocol %$.\n", (uint32_t)interface.iprotocol);
	return false;
}
