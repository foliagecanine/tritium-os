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
	uint8_t valid;
	void *controller;
	uint8_t ctrlr_type;
	uint8_t ctrlrID;
	uint8_t port;
	uint8_t address;
	uint8_t lowspeed;
	uint16_t max_pkt_size;
	void (*driver_function)(uint16_t);
	void *driver0;
	void *driver1;
	void *driver2;
	void *driver3;
} usb_device;

#define USB_DESC_DEVICE		1
#define USB_DESC_CONFIG		2
#define USB_DESC_STRING		3
#define USB_DESC_INTERFACE	4
#define USB_DESC_ENDPOINT	5

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

#define USB_CONFIG_ATTR_REMOTEWAKE 	(1<<5)
#define USB_CONFIG_ATTR_SELFPOWER 	(1<<6)

typedef struct {
	uint8_t length;
	uint8_t type;
	uint16_t total_len;
	uint8_t num_interfaces;
	uint8_t config_value;
	uint8_t config_index;
	uint8_t attributes;
	uint8_t max_power;
} __attribute__((packed)) usb_config_desc;

typedef struct {
	uint8_t length;
	uint8_t type;
	uint8_t interface_num;
	uint8_t alt_setting;
	uint8_t num_endpoints;
	uint8_t iclass;
	uint8_t isubclass;
	uint8_t iprotocol;
	uint8_t interface;
} __attribute__((packed)) usb_interface_desc;

typedef struct {
	uint8_t length;
	uint8_t type;
	uint8_t endpoint_addr;
	uint8_t attributes;
	uint16_t max_pkt_size;
	uint8_t interval;
} __attribute__((packed)) usb_endpoint_desc;

#define USB_MAX_CTRLRS		8

#include <usb/uhci.h>
#include <usb/ehci.h>

#define USB_CTRLR_UHCI		0
#define USB_CTRLR_OHCI		1
#define USB_CTRLR_EHCI		2
#define USB_CTRLR_XHCI		3

#include <usb/hub.h>
#include <usb/hid.h>

#define USB_CLASS_CHECK		0
#define USB_CLASS_HID		3
#define USB_CLASS_HUB		9

void init_usb();
uint16_t usb_dev_addr(uint8_t ctrlrtype, uint8_t ctrlrID, uint8_t devID);
usb_device *usb_device_from_addr(uint16_t dev_addr);
bool usb_assign_address(uint16_t port_addr, uint8_t lowspeed);
bool usb_get_desc(uint16_t dev_addr, void *out, usb_setup_pkt setup_pkt_template, uint16_t size);
usb_dev_desc usb_get_dev_desc(uint16_t dev_addr);
bool usb_get_str_desc(uint16_t dev_addr, void *out, uint8_t index, uint16_t targetlang);
usb_config_desc usb_get_config_desc(uint16_t dev_addr, uint8_t index);
usb_interface_desc usb_get_interface_desc(uint16_t dev_addr, uint8_t config_index, uint8_t interface_index);
usb_endpoint_desc usb_get_endpoint_desc(uint16_t dev_addr, uint8_t config_index, uint8_t interface_index, uint8_t endpoint_index);
bool usb_generic_setup(uint16_t dev_addr, usb_setup_pkt setup_pkt_template);
void *usb_create_interval_in(uint16_t dev_addr, void *out, uint8_t interval, uint8_t endpoint_addr, uint16_t max_pkt_size, uint16_t size);
bool usb_refresh_interval(uint16_t dev_addr, void *data);
void usb_interrupt();

#endif