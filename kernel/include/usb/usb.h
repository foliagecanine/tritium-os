#ifndef _USB_USB_H
#define _USB_USB_H

#include <kernel/stdio.h>
#include <kernel/pci.h>

#define USB_SETUP_GETSTATUS 	0
#define USB_SETUP_CLRFEATURE	1
#define USB_SETUP_SETFEATURE	3
#define USB_SETUP_SETADDRESS	5
#define USB_SETUP_GETDESC		6
#define USB_SETUP_SETDESC		7
#define USB_SETUP_GETCONFIG		8
#define USB_SETUP_SETCONFIG		9

typedef struct {
	uint8_t type;
	uint8_t request;
	uint16_t value;
	uint16_t index;
	uint16_t length;
} __attribute__((packed)) usb_setup_pkt;

typedef struct {
	uint8_t length;
	uint8_t type;
} __attribute__((packed)) usb_desc_header;

typedef struct {
	uint8_t length;
	uint8_t desc_type;
	uint16_t usbver_bcd;
	uint8_t dev_class;
	uint8_t dev_subclass;
	uint8_t dev_protocol;
	uint8_t max_pkt_size;
	uint16_t vendorID;
	uint16_t productID;
	uint16_t releasever_bcd;
	uint8_t manuf_index;
	uint8_t product_index;
	uint8_t sernum_index;
	uint8_t num_configs;
} __attribute__((packed)) usb_dev_desc;

typedef struct {
	uint8_t length;
	uint8_t type;
	uint16_t langid[];
} __attribute__((packed)) usb_str_desc;

#include <usb/uhci.h>
#include <usb/ehci.h>

#define USB_CTRLR_UHCI 0
#define USB_CTRLR_OHCI 1
#define USB_CTRLR_EHCI 2
#define USB_CTRLR_XHCI 3

void init_usb();
uint32_t usb_dev_addr(uint8_t ctrlrtype, uint8_t ctrlrID, uint8_t portID, uint8_t devID);
usb_dev_desc get_usb_dev_desc(uint32_t dev_addr);

#endif