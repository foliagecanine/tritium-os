#ifndef _USB_HID_H
#define _USB_HID_H

#include <usb/usb.h>
#include <stdint.h>
#include <stdbool.h>

bool init_hid(uint16_t dev_addr, usb_config_desc config);
void usb_keyboard_repeat();

#endif