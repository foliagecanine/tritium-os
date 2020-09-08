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

#define XHCI_HCOPS_USBCMD			0x00
#define XHCI_HCOPS_USBCMD_RS		1
#define XHCI_HCOPS_USBCMD_HCRST		(1<<1)
#define XHCI_HCOPS_USBCMD_INTE		(1<<2)
#define XHCI_HCOPS_USBCMD_HSEE		(1<<3)
#define XHCI_HCOPS_USBCMD_LHCRST	(1<<7)
#define XHCI_HCOPS_USBCMD_CSS		(1<<8)
#define XHCI_HCOPS_USBCMD_CRS		(1<<9)
#define XHCI_HCOPS_USBCMD_EWE		(1<<10)

#define XHCI_HCOPS_USBSTS			0x04
#define XHCI_HCOPS_USBSTS_HCHALT	1
#define XHCI_HCOPS_USBSTS_HSE		(1<<2)
#define XHCI_HCOPS_USBSTS_EINT		(1<<3)
#define XHCI_HCOPS_USBSTS_PCD		(1<<4)
#define XHCI_HCOPS_USBSTS_CNR		(1<<11)
#define XHCI_HCOPS_USBSTS_HCE		(1<<12)

#define XHCI_HCOPS_PAGESIZE		0x08
#define XHCI_HCOPS_DNCTRL		0x14
#define XHCI_HCOPS_CRCR			0x18
#define XHCI_HCOPS_DCBAAP		0x30
#define XHCI_HCOPS_CONFIG		0x38
#define XHCI_HCOPS_PORTREGS		0x400

#define XHCI_PORTREGS_PORTSC	0x00
#define XHCI_PORTREGS_PORTSC_CCS		1
#define XHCI_PORTREGS_PORTSC_PE			(1<<1)
#define XHCI_PORTREGS_PORTSC_PORTRST	(1<<4)
#define XHCI_PORTREGS_PORTSC_PORTPWR	(1<<9)
#define XHCI_PORTREGS_PORTSC_PORTSPD(x)	(((x)&0x3C00)>>10)
#define XHCI_PORTREGS_PORTSC_PECHG		(1<<18)
#define XHCI_PORTREGS_PORTSC_PRSTCHG	(1<<21)
#define XHCI_PORTREGS_PORTSC_WARMRST	(1<<31)
#define XHCI_PORTREGS_PORTSC_STATUS		0x00FE0000

#define XHCI_PORTREGS_PORTPMSC	0x04
#define XHCI_PORTREGS_PORTLI	0x08
#define XHCI_PORTREGS_PORTHLPMC	0x0C

#define XHCI_LEGACY_OWNED		(1<<24)
#define XHCI_LEGACY_MASK		((1<<24)|(1<<16))

typedef struct {
	uint8_t legacy; // Legacy flags
	uint8_t phys_portID; // Physical port ID, 0xFF if null
	uint8_t port_pair; // 0xFF if null
} xhci_port;

#define XHCI_PORT_LEGACY_USB2	1
#define XHCI_PORT_LEGACY_HSO	(1<<1)
#define XHCI_PORT_LEGACY_PAIRED	(1<<2)

typedef struct {
	uint64_t param;
	uint32_t status;
	uint32_t command;
} xhci_trb;

#define XHCI_TRB_CYCLE		1
#define XHCI_TRB_EVALTRB	(1<<1)
#define XHCI_TRB_SPE		(1<<2)
#define XHCI_TRB_NOSNOOP	(1<<3)
#define XHCI_TRB_CHAIN		(1<<4)
#define XHCI_TRB_IOC		(1<<5)
#define XHCI_TRB_IMMEDIATE	(1<<6)
#define XHCI_TRB_TRBTYPE(x) ((x)<<10)

#define XHCI_TRBTYPE_NORMAL	1
#define XHCI_TRBTYPE_SETUP	2
#define XHCI_TRBTYPE_DATA	3
#define XHCI_TRBTYPE_STATUS	4
#define XHCI_TRBTYPE_ISO	5
#define XHCI_TRBTYPE_LINK	6
#define XHCI_TRBTYPE_EVENT	7
#define XHCI_TRBTYPE_NOOP	8

typedef struct {
	void *baseaddr;
	void *hcops;
	uint32_t params;
	xhci_trb *cmdring;
	void *dcbaap;
	uint8_t num_ports;
	uint8_t num_ports_2;
	uint8_t num_ports_3;
	uint8_t cycle;
	xhci_port ports[16];
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