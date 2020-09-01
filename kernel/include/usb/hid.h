#ifndef _USB_HID_H
#define _USB_HID_H

#include <kernel/stdio.h>
#include <usb/usb.h>
#include <kernel/kbd.h>

#define USB_HID_KBD_LCTRL	1
#define USB_HID_KBD_LSHIFT	(1<<1)
#define USB_HID_KBD_LALT	(1<<2)
#define USB_HID_KBD_LGUI	(1<<3)
#define USB_HID_KBD_RCTRL	(1<<4)
#define USB_HID_KBD_RSHIFT	(1<<5)
#define USB_HID_KBD_RALT	(1<<6)
#define USB_HID_KBD_RGUI	(1<<7)

bool init_hid(uint16_t dev_addr, usb_config_desc config);
void usb_keyboard_repeat();

#endif