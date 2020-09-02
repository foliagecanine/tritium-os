#ifndef _USB_UHCI_H
#define _USB_UHCI_H

#include <kernel/stdio.h>
#include <usb/usb.h>

#define UHCI_USBCMD			0			// USB Command Register
#define UHCI_USBCMD_MAXP 	(1<<7)		// Max Packet
#define UHCI_USBCMD_CF 		(1<<6)		// Configure
#define UHCI_USBCMD_SWDBG	(1<<5)		// Software Debug
#define UHCI_USBCMD_FGR		(1<<4)		// Force Global Resume
#define UHCI_USBCMD_EGSM	(1<<3)		// Enter Global Suspend Mode
#define UHCI_USBCMD_GRESET	(1<<2)		// Global Reset
#define UHCI_USBCMD_HCRESET	(1<<1)		// Host Controller Reset
#define UHCI_USBCMD_RS		1			// Run/Stop

#define UHCI_USBSTS			2			// USB Status Register
#define UHCI_USBSTS_CTRLHLT (1<<5)		// Controller Halted
#define UHCI_USBSTS_HCPERR 	(1<<4)		// Host Controller Process Error
#define UHCI_USBSTS_HSERR 	(1<<3)		// Host System Error
#define UHCI_USBSTS_RESDT 	(1<<2)		// Resume Detect
#define UHCI_USBSTS_UEINT 	(1<<1)		// USB Error Interrupt
#define UHCI_USBSTS_USBINT 	1			// USB Interrupt
#define UHCI_USBSTS_ALLSTS	0x0000001F

#define UHCI_USBINTR		4			// USB Interrupt Register
#define UHCI_USBINTR_SPIE	(1<<3)		// Short Packet Interrupt Enable
#define UHCI_USBINTR_IOCE	(1<<2)		// Interrupt on Complete Enable
#define UHCI_USBINTR_RIE	(1<<1)		// Resume Interrupt Enable
#define UHCI_USBINTR_TCRCIE	1			// Timeout / CRC Interrupt Enable

#define UHCI_FRNUM			6			// Frame Number Register
#define UHCI_FRNUM_FRNUM	0x000007FF	// Frame Number

#define UHCI_FRBASEADD		8			// Frame Base Address Register
#define UHCI_FRBASEADD_BADD	0xFFFFF000	// Frame Base Address

#define UHCI_SOFMOD			12			// Start of Frame Modifier Register
#define UHCI_SOFMOD_SOFTIM	0x0000007F	// Start of Frame Modifier Timing Value

#define UHCI_PORTSC1		16			// Port 1 Status/Control Register
#define UHCI_PORTSC2		18			// Port 2 Status/Control Register
#define UHCI_PORTSC_SUSPND	(1<<12)		// Port Suspend
#define UHCI_PORTSC_PRESET	(1<<9)		// Port Reset
#define UHCI_PORTSC_LS		(1<<8)		// Low-speed Device
#define UHCI_PORTSC_ALLONE	(1<<7)		// Always 1
#define UHCI_PORTSC_RESDT	(1<<6)		// Resume Detect
#define UHCI_PORTSC_LINSTS	(1<<5)|(1<<4)//Line Status (D+ and D- on the wires)
#define UHCI_PORTSC_PEDCHG	(1<<3)		// Port Enable Changed
#define UHCI_PORTSC_PE		(1<<2)		// Port Enabled Status
#define UHCI_PORTSC_CSTSCHG	(1<<1)		// Port Connection Changed
#define UHCI_PORTSC_CSTS	1			// Port Connection Status

#define UHCI_FRAMEPTR_PTR	0xFFFFFFF0	// Framepointer Pointer Mask
#define UHCI_FRAMEPTR_QUEUE	(1<<1)		// Queue Bit
#define UHCI_FRAMEPTR_TERM	1			// Terminate Bit

// Dword 0 (uhci_usb_xfr_desc.linkptr)
#define UHCI_XFRDESC_LINKPTR 0xFFFFFFF0	// Link Pointer Mask
#define UHCI_XFRDESC_DTOGGLE0 (1<<2)	// Data Toggle Bit (Dword 0)
#define UHCI_XFRDESC_QUEUE	(1<<1)		// Queue Bit
#define UHCI_XFRDESC_TERM	1			// Terminate Bit

// Dword 1 (uhci_usb_xfr_desc.flags0)
#define UHCI_XFRDESC_ACTLEN 0x000003FF	// Actual Length
#define UHCI_XFRDESC_STATUS 0x00FF0000	// Status Byte
#define UHCI_XFRDESC_STATUS_OFF(x) ((x)<<16) // Offset macro for Status Byte
#define UHCI_XFRDESC_STATUS_ROFF(x) ((x)>>16) // Reverse Offset macro for Status Byte
#define UHCI_XFRDESC_STATUS_ACTIVE	UHCI_XFRDESC_STATUS_OFF(0x80)
#define UHCI_XFRDESC_IOC	(1<<24)		// Interrupt on Completion
#define UHCI_XFRDESC_ISO	(1<<25)		// Isochronus Select
#define UHCI_XFRDESC_LS		(1<<26)		// Low Speed Device
#define UHCI_XFRDESC_CERR	0x18000000	// Error count
#define UHCI_XFRDESC_SPD	(1<<29)		// Short Packet Detect Interrupt

// Dword 2 (uhci_usb_xfr_desc.flags1)
#define UHCI_XFRDESC_PID	0x000000FF	// Packet Identifier Byte
#define UHCI_XFRDESC_DEVADDR 0x00007F00	// Device Identifer
#define UHCI_XFRDESC_DEVADDR_OFF(x) ((x)<<8) 	// Device Identifer
#define UHCI_XFRDESC_ENDPT	0x00078000	// Endpoint
#define UHCI_XFRDESC_ENDPT_OFF(x) ((x)<<15)	// Endpoint Offset macro
#define UHCI_XFRDESC_DTOGGLE2 (1<<19)	// Data Toggle Bit (Dword 2)
#define UHCI_XFRDESC_MAXLEN	0xFFE00000	// Max Length
#define UHCI_XFRDESC_MAXLEN_OFF(x) ((x)<<21)	// Offset Macro for Max Length

#define UHCI_PID_SETUP		0x2D		// Setup Packet Identifier Number
#define UHCI_PID_OUT		0xE1		// Data Out Packet Identifier Number
#define UHCI_PID_IN			0x69		// Data In Packet Identifier Number

// Dword 3 (uhci_usb_xfr_desc.bufferptr)
#define UHCI_XFRDESC_BUFPTR	0xFFFFFFFF	// Buffer Pointer


typedef struct {
	uint16_t usbcmd;
	uint16_t usbsts;
	uint16_t usbintr;
	uint16_t frnum;
	uint32_t frbaseadd;
	uint8_t sofmod;
	uint8_t reserved[3];
	volatile uint16_t portsc1;
	volatile uint16_t portsc2;
} uhci_ioregs;

typedef struct {
	uint32_t linkptr;
	uint32_t flags0;
	uint32_t flags1;
	uint32_t bufferptr;
	uint32_t reserved0;
	uint32_t reserved1;
	uint32_t reserved2;
	uint32_t reserved3;
} __attribute__ ((packed)) __attribute__((aligned(16))) uhci_usb_xfr_desc;

typedef struct {
	uint32_t headlinkptr;
	uint32_t elemlinkptr;
	uint32_t taillinkptr;
	uint32_t vaddr;
} __attribute__((packed)) __attribute__((aligned(16))) uhci_usb_queue;

typedef struct {
	uint16_t iobase;
	uint32_t framelist_paddr;
	uint32_t framelist_vaddr;
	uhci_usb_queue *queues_vaddr;
	uhci_usb_queue *queues_paddr;
	uint8_t num_ports;
	usb_device devices[128];
} uhci_controller;

uhci_controller *get_uhci_controller(uint8_t id);
bool uhci_set_address(usb_device *device, uint8_t dev_address);
bool uhci_assign_address(uint8_t ctrlrID, uint8_t port, uint8_t lowspeed);
bool uhci_generic_setup(usb_device *device, usb_setup_pkt setup_pkt_template);
bool uhci_usb_get_desc(usb_device *device, void *out, usb_setup_pkt setup_pkt_template, uint16_t size);
void *uhci_create_interval_in(usb_device *device, void *out, uint8_t interval, uint8_t endpoint_addr, uint16_t max_pkt_size, uint16_t size);
bool uhci_refresh_interval(void *data);
void uhci_destroy_interval(void *data);
usb_dev_desc uhci_get_usb_dev_descriptor(usb_device *device, uint16_t size);
bool uhci_get_usb_str_desc(usb_device *device, char *out, uint8_t index, uint16_t targetlang);
usb_config_desc uhci_get_config_desc(usb_device *device, uint8_t index);
usb_interface_desc uhci_get_interface_desc(usb_device *device, uint8_t config_index, uint8_t interface_index);
usb_endpoint_desc uhci_get_endpoint_desc(usb_device *device, uint8_t config_index, uint8_t interface_index, uint8_t endpoint_index);
uint8_t uhci_get_unused_device(uhci_controller *uc);
uint8_t init_uhci_ctrlr(uint16_t iobase);

#endif