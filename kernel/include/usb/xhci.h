#ifndef _USB_XHCI_H
#define _USB_XHCI_H

#include <kernel/stdio.h>
#include <usb/usb.h>

#define XHCI_HCCAP_CAPLEN		0x00
#define XHCI_HCCAP_HCIVER		0x02
#define XHCI_HCCAP_HCSPARAM1	0x04
#define XHCI_HCCAP_HCSPARAM2	0x08
#define XHCI_HCCAP_HCSPARAM3	0x0C
#define XHCI_HCCAP_HCCPARAM1	0x10
#define XHCI_HCCAP_DBOFF		0x14
#define XHCI_HCCAP_RTSOFF		0x18
#define XHCI_HCCAP_HCCPARAM2	0x1C

#define XHCI_HCOPS_USBCMD		0x00
#define XHCI_HCOPS_USBCMD_RS		0
#define XHCI_HCOPS_USBCMD_HCRST		1
#define XHCI_HCOPS_USBCMD_INTE		2
#define XHCI_HCOPS_USBCMD_HSEE		3
#define XHCI_HCOPS_USBCMD_LHCRST	7
#define XHCI_HCOPS_USBCMD_CSS		8
#define XHCI_HCOPS_USBCMD_CRS		9
#define XHCI_HCOPS_USBCMD_EWE		10

#define XHCI_HCOPS_USBSTS		0x04
#define XHCI_HCOPS_USBSTS_HCHALT	0
#define XHCI_HCOPS_USBSTS_HSE		2
#define XHCI_HCOPS_USBSTS_EINT		3
#define XHCI_HCOPS_USBSTS_PCD		4
#define XHCI_HCOPS_USBSTS_CNR		11
#define XHCI_HCOPS_USBSTS_HCE		12

#define XHCI_HCOPS_PAGESIZE		0x08
#define XHCI_HCOPS_DNCTRL		0x14
#define XHCI_HCOPS_CRCR			0x18
#define XHCI_HCOPS_DCBAAP		0x30
#define XHCI_HCOPS_CONFIG		0x38
#define XHCI_HCOPS_PORTREGS		0x400

#define XHCI_LEGACY_OWNED		(1<<24)
#define XHCI_LEGACY_MASK		((1<<24)|(1<<16))

typedef struct {
	void *baseaddr;
	void *hcops;
	uint8_t num_ports;
	usb_device devices[128];
} xhci_controller;

xhci_controller *get_xhci_controller(uint8_t id);
bool xhci_set_address(usb_device *device, uint8_t dev_address);
bool xhci_assign_address(uint8_t ctrlrID, uint8_t port);
bool xhci_generic_setup(usb_device *device, usb_setup_pkt setup_pkt_template);
bool xhci_usb_get_desc(usb_device *device, void *out, usb_setup_pkt setup_pkt_template, uint16_t size);
void *xhci_create_interval_in(usb_device *device, void *out, uint8_t interval, uint8_t endpoint_addr, uint16_t max_pkt_size, uint16_t size);
bool xhci_refresh_interval(void *data);
void xhci_destroy_interval(void *data);
usb_dev_desc xhci_get_usb_dev_descriptor(usb_device *device, uint16_t size);
bool xhci_get_usb_str_desc(usb_device *device, char *out, uint8_t index, uint16_t targetlang);
usb_config_desc xhci_get_config_desc(usb_device *device, uint8_t index);
usb_interface_desc xhci_get_interface_desc(usb_device *device, uint8_t config_index, uint8_t interface_index);
usb_endpoint_desc xhci_get_endpoint_desc(usb_device *device, uint8_t config_index, uint8_t interface_index, uint8_t endpoint_index);
uint8_t xhci_get_unused_device(xhci_controller *xc);
uint8_t init_xhci_ctrlr(uint32_t baseaddr);

#endif