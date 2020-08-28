#ifndef _USB_HUB_H
#define _USB_HUB_H

#include <kernel/stdio.h>
#include <usb/usb.h>

#define USB_DESC_HUB			0x29

#define HUB_FEATURE_PORTPOWER	8
#define HUB_FEATURE_PORTCNCTN	0x10
#define HUB_FEATURE_RESET		4
#define HUB_FEATURE_CRESET		0x14

#define HUB_STATUS_CONNECTION	1
#define HUB_STATUS_ENABLE		(1<<1)
#define HUB_STATUS_LS			(1<<8)
#define HUB_STATUS_CNCTSTSCHG	(1<<24)
#define HUB_STATUS_PECHANGE		(1<<25)
#define HUB_STATUS_RESET		(1<<28)

typedef struct {
	uint8_t length;
	uint8_t type;
	uint8_t num_ports;
	uint16_t hub_chars;
	uint8_t potpgt;
	uint8_t max_current;
} __attribute__((packed)) usb_hub_desc;

bool init_hub(uint16_t dev_addr, usb_config_desc config);

#endif