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
#define XHCI_HCOPS_USBSTS_SRE		(1<<10)
#define XHCI_HCOPS_USBSTS_CNR		(1<<11)
#define XHCI_HCOPS_USBSTS_HCE		(1<<12)

#define XHCI_DBOFF_DBOFF		0xFFFFFFF8

#define XHCI_RTSOFF_RTSOFF		0xFFFFFFE0

#define XHCI_HCOPS_PAGESIZE		0x08
#define XHCI_HCOPS_DNCTRL		0x14

#define XHCI_HCOPS_CRCR			0x18
#define XHCI_HCOPS_CRCR_RCS		1

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
#define XHCI_TRB_CHAIN		(1<<4)
#define XHCI_TRB_IOC		(1<<5)
#define XHCI_TRB_IMMEDIATE	(1<<6)
#define XHCI_TRB_TRBTYPE(x) ((x)<<10)
#define XHCI_TRB_GTRBTYPE(x) (((x)>>10)&0x1F)

#define XHCI_TRB_EVENT		(1<<2)

#define XHCI_TRB_COMMAND_SPE		(1<<2)
#define XHCI_TRB_COMMAND_NOSNOOP	(1<<3)
#define XHCI_TRB_COMMAND_BLOCK		(1<<9)
#define XHCI_TRB_COMMAND_SLOT(x)	((x)<<24)

#define XHCI_TRB_SETUP_DIRECTION(x)	((x)<<16)

#define XHCI_TRB_DATA_DIRECTION(x)	((x)<<16)
#define XHCI_TRB_DATA_REMAINING(x)	((x)<<17)

#define XHCI_TRBTYPE_NORMAL	1
#define XHCI_TRBTYPE_SETUP	2
#define XHCI_TRBTYPE_DATA	3
#define XHCI_TRBTYPE_STATUS	4
#define XHCI_TRBTYPE_ISO	5
#define XHCI_TRBTYPE_LINK	6
#define XHCI_TRBTYPE_EVENT	7
#define XHCI_TRBTYPE_NOOP	8
#define XHCI_TRBTYPE_ENSLOT	9
#define XHCI_TRBTYPE_ADDR	11

#define XHCI_TRBTYPE_EVT_COMPLETE	32

#define XHCI_DIRECTION_NONE	0
#define XHCI_DIRECTION_OUT	2
#define XHCI_DIRECTION_IN	3

#define XHCI_DATA_DIR_OUT	0
#define XHCI_DATA_DIR_IN	1

#define XHCI_EVTTRB_STATUS_GETCODE(x) 	((x)>>24)
#define XHCI_EVTTRB_COMMAND_GETSLOT(x) 	((x)>>24)
#define XHCI_TRBCODE_SUCCESS 1

#define XHCI_RUNTIME_IR0	0x20

#define XHCI_INTREG_IMR		0
#define XHCI_INTREG_IMR_IP	1
#define XHCI_INTREG_IMR_EN	(1<<1)

#define XHCI_INTREG_IMOD	0x04
#define XHCI_INTREG_IMOD_IMI 0x0000FFFF
#define XHCI_INTREG_IMOD_IMC 0xFFFF0000

#define XHCI_INTREG_ERSTS	0x08
#define XHCI_INTREG_ERSTBA	0x10

#define XHCI_INTREG_ERDQPTR	0x18
#define XHCI_INTREG_ERDQPTR_EHBSY	(1<<3)

typedef struct {
	void *baseaddr;
	void *hcops;
	void *runtime;
	void *dboff;
	uint64_t *dcbaap;
	uint32_t params;
	uint8_t ctx_size;
	xhci_trb *cmdring;
	xhci_trb *ccmdtrb;
	uint8_t cmdcycle;
	xhci_trb *evtring;
	xhci_trb *cevttrb;
	uint8_t evtcycle;
	void *evtring_table;
	void *evtring_alloc;
	uint8_t num_ports;
	uint8_t num_ports_2;
	uint8_t num_ports_3;
	uint8_t ctrlrID;
	xhci_port ports[16];
	usb_device devices[128];
} xhci_controller;

typedef struct {
	uint32_t entries;
	uint16_t max_exit_latency;
	uint8_t rh_port_num;
	uint8_t num_ports;
	uint16_t tt;
	uint16_t int_target;
	uint8_t devaddr;
	uint16_t reserved;
	uint8_t state;
	uint32_t reserved0;
	uint32_t reserved1;
	uint32_t reserved2;
	uint32_t reserved3;
} __attribute__((packed)) xhci_slot;

#define XHCI_SLOT_ENTRY_ROUTESTRING(x)	(x)
#define XHCI_SLOT_ENTRY_SPEED(x)		((x)<<20)
#define XHCI_SLOT_ENTRY_MULTITT				(1<<25)
#define XHCI_SLOT_ENTRY_HUB				(1<<26)
#define XHCI_SLOT_ENTRY_COUNT(x)		((x)<<27)

#define XHCI_SLOT_STATE_D_E				0
#define XHCI_SLOT_STATE_DEFAULT			1
#define XHCI_SLOT_STATE_ADDRESSED		2
#define XHCI_SLOT_STATE_CONFIGURED		3

typedef struct {
	uint16_t state;
	uint8_t interval;
	uint8_t esinterval_h;
	uint8_t flags;
	uint8_t maxburst;
	uint16_t max_pkt_size;
	uint64_t dqptr;
	uint16_t avg_trblen;
	uint16_t exinterval_l;
	uint32_t reserved0;
	uint32_t reserved1;
	uint32_t reserved2;
} __attribute__((packed)) xhci_endpt;

#define XHCI_ENDPT_STATE_STATE(x)		(x)	
#define XHCI_ENDPT_STATE_MULTI(x)		((x)<<8)
#define XHCI_ENDPT_STATE_PSTREAMS(x)	((x)<<10)
#define XHCI_ENDPT_STATE_LSA			(1<<15)

#define XHCI_ENDPT_FLAGS_ERRCNT(x)		((x)<<1)
#define XHCI_ENDPT_FLAGS_ENDPT_TYPE(x)	((x)<<3)
#define XHCI_ENDPT_FLAGS_HID			(1<<7)

#define XHCI_ENDPOINT_STATE_DISABLE		0
#define XHCI_ENDPOINT_STATE_RUN			1
#define XHCI_ENDPOINT_STATE_HALT		2
#define XHCI_ENDPOINT_STATE_STOP		3
#define XHCI_ENDPOINT_STATE_ERROR		4

typedef struct {
	void *slot_template;
	xhci_slot *slot_ctx;
	
	xhci_trb *endpt_ring[31];
	xhci_trb *cendpttrb[31];
	uint8_t endpt_cycle[31];
} __attribute__((packed)) xhci_devdata;

xhci_controller *get_xhci_controller(uint8_t id);
xhci_trb xhci_send_cmdtrb(xhci_controller *xc, xhci_trb trb);
bool xhci_set_address(usb_device *device, uint32_t command_params);
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
uint8_t init_xhci_ctrlr(void *baseaddr, uint8_t irq);

#endif