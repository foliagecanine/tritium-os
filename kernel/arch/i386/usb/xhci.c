#include <usb/xhci.h>

//#define DEBUG
//#define DEBUG_TERMINAL
//#define DEBUG_DATA

#ifdef DEBUG
#ifdef DEBUG_TERMINAL
#define dbgprintf printf
#else
#define dbgprintf dprintf
#endif
#define dbgdump_memory dump_memory
#define dbgdump_memory32 dump_memory32
#ifdef DEBUG_DATA
#define DATA dbgprintf
#else
#define DATA(x, ...) asm("nop")
#endif
#else
#define dbgprintf noprintf
#define dbgdump_memory(x, y) asm("nop")
#define dbgdump_memory32(x, y) asm("nop")
#define DATA(x, ...) asm("nop")
#endif

#define ERROR(x) dprintf("[xHCI] %s", x) & 0

xhci_controller xhci_controllers[USB_MAX_CTRLRS];
uint8_t         xhci_num_ctrlrs = 0;

// Force a 64 bit read
inline uint64_t _rd64(volatile void *mem) { return *(volatile uint64_t *)mem; }

// Force a 32 bit read
inline uint32_t _rd32(volatile void *mem) { return *(volatile uint32_t *)mem; }

// Force a 16 bit read
inline uint16_t _rd16(volatile void *mem) { return (uint16_t)((*(volatile uint32_t *)(void *)((uint32_t)mem & ~3)) >> (((uint32_t)mem & 3) * 8)); }

// Force an 8 bit read
inline uint8_t _rd8(volatile void *mem) { return (uint8_t)((*(volatile uint32_t *)(void *)((uint32_t)mem & ~3)) >> (((uint32_t)mem & 3) * 8)); }

// Force a 32 bit write
inline void _wr32(void *mem, uint32_t b) { __asm__ __volatile__("movl %%eax, %0" : "=m"(*(uint32_t *)mem) : "a"(b) : "memory"); }

// Emulate a 64 bit write
inline void _wr64(void *mem, uint64_t b, xhci_controller *xc) {
	_wr32(mem, (uint32_t)b);
	if (xc->params & 1)
		_wr32(mem + 4, (uint32_t)(b >> 32));
}

void ring_doorbell(xhci_controller *xc, uint8_t slot, uint32_t val) {
	DATA("[xHCI] Ringing doorbell %$ with value %$\n", (uint32_t)slot, val);
	_wr32(xc->dboff + (slot * sizeof(uint32_t)), val);
}

bool xhci_global_reset(xhci_controller *xc) {
	uint32_t *dump = (uint32_t *)xc->hcops;
	for (uint8_t i = 0; i < 4; i++) {
		dbgprintf("%lX %lX %lX %lX\n", dump[0 + (i * 4)], dump[1 + (i * 4)], dump[2 + (i * 4)], dump[3 + (i * 4)]);
	}

	dbgprintf("[xHCI] Clear RunStop\n");

	// Clear the RunStop bit and wait until the controller stops
	_wr32(xc->hcops + XHCI_HCOPS_USBCMD, _rd32(xc->hcops + XHCI_HCOPS_USBCMD) & ~XHCI_HCOPS_USBCMD_RS);
	uint16_t timeout = 20;
	while (!(_rd32(xc->hcops + XHCI_HCOPS_USBSTS) & XHCI_HCOPS_USBSTS_HCHALT)) {
		sleep(1);
		if (!--timeout)
			return ERROR("RunStop bit failed to clear.");
	}

	dbgprintf("[xHCI] Cleared RunStop\n[xHCI] Resetting xHCI.\n");

	// Set the HCReset bit and wait until it clears itself
	timeout = 1000;
	_wr32(xc->hcops + XHCI_HCOPS_USBCMD, XHCI_HCOPS_USBCMD_HCRST);
	while (_rd32(xc->hcops + XHCI_HCOPS_USBCMD) & XHCI_HCOPS_USBCMD_HCRST || _rd32(xc->hcops + XHCI_HCOPS_USBSTS) & XHCI_HCOPS_USBSTS_CNR) {
		sleep(1);
		if (!--timeout)
			return ERROR("HCReset bit failed to clear.");
	}
	dbgprintf("[xHCI] Reset xHCI Controller Globally\n");
	sleep(50);

	uint32_t paramoff = _rd16(xc->baseaddr + XHCI_HCCAP_HCCPARAM1 + 2) * 4;
	uint32_t paramval = _rd32(xc->baseaddr + paramoff);
	while ((paramval & 0xFF) && (paramval & 0xFF) != 1) {
		if ((paramval >> 8) & 0xFF) {
			paramoff += ((paramval >> 8) & 0xFF) * 4;
			paramval = _rd32(xc->baseaddr + paramoff);
		} else {
			paramval = 0;
		}
	}
	if (paramval & 0xFF) {
		_wr32(xc->baseaddr + paramoff, _rd32(xc->baseaddr + paramoff) | XHCI_LEGACY_OWNED);
		timeout = 10;
		while ((_rd32(xc->baseaddr + paramoff) & XHCI_LEGACY_MASK) != XHCI_LEGACY_OWNED) {
			sleep(1);
			if (!--timeout)
				return ERROR("BIOS Ownership failed to disable.");
			;
		}
		dbgprintf("[xHCI] BIOS Ownership released\n");
	} else {
		dbgprintf("[xHCI] No BIOS Ownership detected\n");
	}

	dbgprintf("[xHCI] Reset xHCI Globally\n");

	return true;
}

void xhci_pair_ports(xhci_controller *xc) {
	for (uint8_t i = 0; i < 16; i++) {
		xc->ports[i].port_pair = 0xFF;
		xc->ports[i].phys_portID = 0xFF;
	}

	uint32_t paramoff = _rd16(xc->baseaddr + XHCI_HCCAP_HCCPARAM1 + 2) * 4;
	uint32_t paramval = _rd32(xc->baseaddr + paramoff);

	while (paramoff) {
		if ((paramval & 0xFF) == 2) {
			if (_rd8(xc->baseaddr + paramoff + 3) == 2) {
				uint8_t count = _rd8(xc->baseaddr + paramoff + 9);
				for (uint8_t i = 0; i < count; i++) {
					xc->ports[_rd8(xc->baseaddr + paramoff + 8) + i - 1].phys_portID = xc->num_ports_2++;
					xc->ports[_rd8(xc->baseaddr + paramoff + 8) + i - 1].legacy |= XHCI_PORT_LEGACY_USB2;
					if (_rd16(xc->baseaddr + paramoff + 10) & 2)
						xc->ports[_rd8(xc->baseaddr + paramoff + 8) + i - 1].legacy |= XHCI_PORT_LEGACY_HSO;
				}
			} else if (_rd8(xc->baseaddr + paramoff + 3) == 3) {
				uint8_t count = _rd8(xc->baseaddr + paramoff + 9);
				for (uint8_t i = 0; i < count; i++) {
					xc->ports[_rd8(xc->baseaddr + paramoff + 8) + i - 1].phys_portID = xc->num_ports_3++;
				}
			}
		}
		paramoff += ((paramval >> 8) & 0xFF) * 4;
		if (paramval & 0xFF00)
			paramval = _rd32(xc->baseaddr + paramoff);
		else
			paramoff = 0;
	}

	dbgprintf("[xHCI] Found %$ USB 2 ports and %$ USB 3 ports.\n", xc->num_ports_2, xc->num_ports_3);

	for (uint8_t i = 0; i < 16; i++) {
		if (xc->ports[i].phys_portID != 0xFF) {
			for (uint8_t j = 0; j < 16; j++) {
				if (xc->ports[j].phys_portID != 0xFF) {
					if (i != j) {
						if (xc->ports[i].phys_portID == xc->ports[j].phys_portID) {
							xc->ports[i].port_pair = j;
							xc->ports[i].legacy |= XHCI_PORT_LEGACY_PAIRED;
							xc->ports[j].port_pair = i;
							xc->ports[j].legacy |= XHCI_PORT_LEGACY_PAIRED;
						}
					}
				}
			}
			uint8_t legacy = xc->ports[i].legacy;
			dbgprintf("[xHCI] Port %$: %s%s%s, Phys %$, PortPair %$\n", (uint32_t)i, legacy & 1 ? "USB 2" : "USB 3", legacy & 2 ? " HSO" : "", legacy & 4 ? " Paired" : "", (uint32_t)xc->ports[i].phys_portID, (uint32_t)xc->ports[i].port_pair);
		}
	}
}

bool xhci_port_reset(xhci_controller *xc, uint8_t port) {
	void *   portbase = xc->hcops + XHCI_HCOPS_PORTREGS + (port * 16);
	uint32_t writeflags = 0;

	// Give the port power
	if (!(_rd32(portbase + XHCI_PORTREGS_PORTSC) & XHCI_PORTREGS_PORTSC_PORTPWR)) {
		_wr32(portbase + XHCI_PORTREGS_PORTSC, XHCI_PORTREGS_PORTSC_PORTPWR);
		sleep(20);
		if (!(_rd32(portbase + XHCI_PORTREGS_PORTSC) & XHCI_PORTREGS_PORTSC_PORTPWR)) {
			dbgprintf("[xHCI] Failed to init power on port %u\n", port);
			return false;
		}
		writeflags |= XHCI_PORTREGS_PORTSC_PORTPWR;
	}

	// Clear the status bits (but keep the power bit on)
	_wr32(portbase + XHCI_PORTREGS_PORTSC, writeflags | XHCI_PORTREGS_PORTSC_STATUS);

	uint16_t timeout;

	// Do regular reset with USB 2 or a warm reset with USB 3
	if (xc->ports[port].legacy & XHCI_PORT_LEGACY_USB2) {
		_wr32(portbase + XHCI_PORTREGS_PORTSC, writeflags | XHCI_PORTREGS_PORTSC_PORTRST);
		timeout = 3;
	} else {
		_wr32(portbase + XHCI_PORTREGS_PORTSC, writeflags | XHCI_PORTREGS_PORTSC_WARMRST);
		timeout = 500;
	}

	while (!(_rd32(portbase + XHCI_PORTREGS_PORTSC) & XHCI_PORTREGS_PORTSC_PRSTCHG)) {
		sleep(1);
		if (!timeout--) {
			dbgprintf("[xHCI] No port reset change on %u\n", port);
			return false;
		}
	}

	sleep(3);

	if (_rd32(portbase + XHCI_PORTREGS_PORTSC) & XHCI_PORTREGS_PORTSC_PE) {
		_wr32(portbase + XHCI_PORTREGS_PORTSC, writeflags | XHCI_PORTREGS_PORTSC_STATUS);
		return true;
	}

	dprintf("[xHCI] Port %u not enabled.\n", port);
	return false;
}

void xhci_interrupt() {
	DATA("[xHCI] USB INTERRUPT\n");
	for (uint8_t i = 0; i < USB_MAX_CTRLRS; i++) {
		xhci_controller *xc = get_xhci_controller(i);
		if (!xc->hcops)
			continue;
		if (!_rd32(xc->hcops + XHCI_HCOPS_USBSTS))
			continue;
		_wr32(xc->hcops + XHCI_HCOPS_USBSTS, _rd32(xc->hcops + XHCI_HCOPS_USBSTS));
		if ((_rd32(xc->runtime + XHCI_RUNTIME_IR0 + XHCI_INTREG_IMR) & (XHCI_INTREG_IMR_EN | XHCI_INTREG_IMR_IP)) == (XHCI_INTREG_IMR_EN | XHCI_INTREG_IMR_IP)) {
			_wr32(xc->runtime + XHCI_RUNTIME_IR0 + XHCI_INTREG_IMR, _rd32(xc->runtime + XHCI_RUNTIME_IR0 + XHCI_INTREG_IMR));
			while ((xc->cevttrb->command & XHCI_TRB_CYCLE) == xc->evtcycle) {
				if (xc->cevttrb->command & XHCI_TRB_EVENT) {
					DATA("[xHCI] DATA: Detected Event TRB\n");
					uint32_t *status = map_paddr((void *)(uint32_t)xc->cevttrb->param, 1);
					// dbgprintf("Writing %# to %#\n",(uint64_t)xc->cevttrb->status,xc->cevttrb->param);
					*status = xc->cevttrb->status;
					unmap_secret_page(status, 1);
				} else if (xc->cevttrb->command != 0x8801) {
					xhci_trb *exec_trb = map_paddr((void *)(uint32_t)xc->cevttrb->param, 1); //(xc->cevttrb->param&0xFFF)+((void *)xc->cmdring);
					exec_trb->param = (uint64_t)(uint32_t)(void *)xc->cevttrb;
					unmap_secret_page(exec_trb, 1);
				}
				DATA("[xHCI] DATA: Processed TRB\n");
				DATA("[xHCI] DATA: Param: %#\n", xc->cevttrb->param);
				DATA("[xHCI] DATA: Status: %#\n", (uint64_t)xc->cevttrb->status);
				DATA("[xHCI] DATA: Command: %#\n", (uint64_t)xc->cevttrb->command);
				xc->cevttrb++;
				if ((void *)(xc->cevttrb) > (void *)xc->evtring + 65536) {
					xc->cevttrb = xc->evtring;
					xc->evtcycle ^= 1;
				}
			}
			DATA("[xHCI] DATA: New ERDQPTR: %#\n", (uint64_t)(uint32_t)xc->cevttrb);
			_wr64(xc->runtime + XHCI_RUNTIME_IR0 + XHCI_INTREG_ERDQPTR, ((uint64_t)(uint32_t)get_phys_addr(xc->cevttrb)) | XHCI_INTREG_ERDQPTR_EHBSY, xc);
		}
		for (uint8_t j = 0; j < 127; j++) {
			if (xhci_controllers[i].devices[j].valid && xhci_controllers[i].devices[j].driver_function) {
				xhci_controllers[i].devices[j].driver_function(usb_dev_addr(USB_CTRLR_XHCI, i, j));
			}
		}
	}
}

void *xhci_init_device(usb_device *usbdev, xhci_controller *xc, uint32_t routing) {
	dbgprintf("Routing: %X\n", routing);
	xhci_devdata *data = usbdev->data;
	data->slot_ctx = alloc_page(1);
	memset(data->slot_ctx, 0, 4096);
	xhci_slot *this_slot = data->slot_ctx;
	*(uint64_t *)(xc->dcbaap + (usbdev->slot * 8)) = (uint64_t)(uint32_t)(void *)this_slot;

	xhci_slot slot;
	slot.entries = XHCI_SLOT_ENTRY_COUNT(1) | XHCI_SLOT_ENTRY_SPEED(usbdev->speed) | XHCI_SLOT_ENTRY_ROUTESTRING(routing & XHCI_HUB_ROUTE);
	slot.rh_port_num = usbdev->port + 1;
	slot.max_exit_latency = 0;
	slot.int_target = 0; // IR_0
	slot.state = XHCI_SLOT_STATE_D_E;
	slot.devaddr = 0;

	*this_slot = slot;

	xhci_endpt ep;
	memset(&ep, 0, sizeof(xhci_endpt));

	ep.max_pkt_size = usbdev->max_pkt_size;
	ep.state = XHCI_ENDPT_STATE_STATE(XHCI_ENDPOINT_STATE_DISABLE);
	ep.flags = XHCI_ENDPT_FLAGS_ENDPT_TYPE(4) | XHCI_ENDPT_FLAGS_ERRCNT(3);
	ep.avg_trblen = 8;

	data->endpt_ring[0] = alloc_page(1);
	data->cendpttrb[0] = data->endpt_ring[0];
	ep.dqptr = (uint64_t)(uint32_t)get_phys_addr(data->endpt_ring[0]) | XHCI_TRB_CYCLE;
	memset(data->endpt_ring[0], 0, 4096);
	data->endpt_ring[0][127].param = ep.dqptr;
	data->endpt_ring[0][127].status = 0;
	data->endpt_ring[0][127].command = XHCI_TRB_TRBTYPE(XHCI_TRBTYPE_LINK);
	data->endpt_cycle[0] = XHCI_TRB_CYCLE;

	xhci_endpt *this_ep = ((void *)data->slot_ctx) + xc->ctx_size;
	*this_ep = ep;

	xc->dcbaap[usbdev->slot] = (uint64_t)(uint32_t)get_phys_addr(data->slot_ctx);

	return data->slot_ctx;
}

const uint16_t default_speeds[] = {64, 8, 64, 512};

bool xhci_init_port_dev(xhci_controller *xc, uint8_t port, uint32_t routing, uint8_t speed) {
	dbgprintf("[xHCI] Attempting to initialize device\n");
	void *   portbase = xc->hcops + XHCI_HCOPS_PORTREGS + (port * 16);
	xhci_trb trb;
	trb.param = 0;
	trb.status = 0;
	trb.command = XHCI_TRB_TRBTYPE(XHCI_TRBTYPE_ENSLOT);

	trb = xhci_send_cmdtrb(xc, trb);
	if (XHCI_EVTTRB_STATUS_GETCODE(trb.status) != XHCI_TRBCODE_SUCCESS)
		return ERROR("Failed when enabling slot: ") & dbgprintf("Error code 0x%X\n", XHCI_EVTTRB_STATUS_GETCODE(trb.status));

	usb_device *usbdev = &xc->devices[0];
	memset(usbdev, 0, sizeof(usb_device));
	usbdev->slot = XHCI_EVTTRB_COMMAND_GETSLOT(trb.command);
	usbdev->port = port;
	usbdev->routing = routing;
	if (speed == 5)
		usbdev->speed = XHCI_PORTREGS_PORTSC_PORTSPD(_rd32(portbase + XHCI_PORTREGS_PORTSC));
	else
		usbdev->speed = speed;
	usbdev->controller = xc;
	usbdev->ctrlrID = xc->ctrlrID;
	usbdev->ctrlr_type = USB_CTRLR_XHCI;
	usbdev->max_pkt_size = default_speeds[usbdev->speed];
	dbgprintf("[xHCI] Max Packet Size: %u\n", usbdev->max_pkt_size);
	usbdev->data = alloc_page(1);
	xhci_devdata *data = usbdev->data;

	xhci_init_device(usbdev, xc, routing);
	data->slot_template = alloc_page(1);
	memset(data->slot_template, 0, 4096);
	memcpy((void *)(data->slot_template) + xc->ctx_size, data->slot_ctx, 2048);
	((uint32_t *)data->slot_template)[1] = 3;
	// xhci_set_address(usbdev,0);
	if (!xhci_set_address(usbdev, 0))
		return ERROR("Set address failed.\n");

	usbdev->address = data->slot_ctx->devaddr;
	memcpy(&xc->devices[usbdev->address], usbdev, sizeof(usb_device));
	usbdev = &xc->devices[usbdev->address];
	usbdev->valid = true;

	usb_dev_desc devdesc = xhci_get_usb_dev_descriptor(usbdev, 8);

	if (!devdesc.length)
		return ERROR("Device Descriptor failed.\n");

	// dbgprintf("[xHCI] Got Descriptor\n");
	// dbgprintf("[xHCI] Length: %$\n",(uint32_t)devdesc.length);
	// dbgprintf("[xHCI]   Type: %$\n",(uint32_t)devdesc.desc_type);
	// dbgprintf("[xHCI] MaxPkt: %$\n",(uint32_t)devdesc.max_pkt_size);

	dbgprintf("[xHCI] Retrieving full descriptor...\n");

	usbdev->max_pkt_size = devdesc.max_pkt_size;
	devdesc = xhci_get_usb_dev_descriptor(usbdev, devdesc.length);

	if (!devdesc.length)
		return false;

	dbgprintf("[xHCI] Got Descriptor\n");
	dbgprintf("[xHCI]   Length: %$\n", (uint32_t)devdesc.length);
	dbgprintf("[xHCI]     Type: %$\n", (uint32_t)devdesc.desc_type);
	dbgprintf("[xHCI]  Version: %#\n", (uint64_t)devdesc.usbver_bcd);
	dbgprintf("[xHCI]    Class: %$\n", (uint32_t)devdesc.dev_class);
	dbgprintf("[xHCI] Subclass: %$\n", (uint32_t)devdesc.dev_subclass);
	dbgprintf("[xHCI] Protocol: %$\n", (uint32_t)devdesc.dev_protocol);
	dbgprintf("[xHCI] MaxPktSz: %$\n", (uint32_t)devdesc.max_pkt_size);
	dbgprintf("[xHCI] VendorID: %#\n", (uint64_t)devdesc.vendorID);
	dbgprintf("[xHCI] PrductID: %#\n", (uint64_t)devdesc.productID);
	dbgprintf("[xHCI] ReleaseV: %#\n", (uint64_t)devdesc.releasever_bcd);
	dbgprintf("[xHCI] ManufIdx: %$\n", (uint32_t)devdesc.manuf_index);
	dbgprintf("[xHCI] PrdctIdx: %$\n", (uint32_t)devdesc.product_index);
	dbgprintf("[xHCI] SrialIdx: %$\n", (uint32_t)devdesc.sernum_index);
	dbgprintf("[xHCI]  NumCfgs: %$\n", (uint32_t)devdesc.num_configs);

	return true;
}

uint8_t init_xhci_ctrlr(void *baseaddr, uint8_t irq) {
	dbgprintf("[xHCI] Base Address: %#\n", (uint64_t)(uint32_t)baseaddr);
	for (uint8_t i = 0; i < 16; i++) {
		identity_map((void *)baseaddr + (i * 4096));
	}

	// Populate generic addresses
	xhci_controller *this_ctrlr = get_xhci_controller(xhci_num_ctrlrs++);
	this_ctrlr->ctrlrID = xhci_num_ctrlrs - 1;
	this_ctrlr->baseaddr = (void *)baseaddr;
	this_ctrlr->hcops = (void *)baseaddr + _rd8(this_ctrlr->baseaddr + XHCI_HCCAP_CAPLEN);
	this_ctrlr->runtime = (void *)baseaddr + (_rd32(baseaddr + XHCI_HCCAP_RTSOFF) & XHCI_RTSOFF_RTSOFF);
	this_ctrlr->dboff = (void *)baseaddr + (_rd32(baseaddr + XHCI_HCCAP_DBOFF) & XHCI_DBOFF_DBOFF);
	this_ctrlr->params = _rd32(this_ctrlr->baseaddr + XHCI_HCCAP_HCCPARAM1);
	this_ctrlr->ctx_size = (this_ctrlr->params & 4) ? 64 : 32;
	dbgprintf("[xHCI] Ops Address: %#\n", (uint64_t)(uint32_t)this_ctrlr->hcops);

	// Check to make sure this is a valid controller
	uint16_t hcver = _rd16(this_ctrlr->baseaddr + XHCI_HCCAP_HCIVER);
	if (hcver < 0x95)
		return false;
	else
		dbgprintf("[xHCI] Controller version: %#\n", (uint64_t)hcver);

	// Reset xHCI controller
	if (!xhci_global_reset(this_ctrlr))
		return ERROR("[xHCI] Failed to reset xHCI Globally\n");

	// Count total ports
	this_ctrlr->num_ports = _rd32(this_ctrlr->baseaddr + XHCI_HCCAP_HCSPARAM1) >> 24;
	dbgprintf("[xHCI] Detected %u ports\n", this_ctrlr->num_ports);

	// Pair each USB 3 port with their USB 2 port
	xhci_pair_ports(this_ctrlr);

	// Allocate space for the device context base address array
	this_ctrlr->dcbaap = alloc_page(1);
	memset(this_ctrlr->dcbaap, 0, 4096);
	_wr64(this_ctrlr->hcops + XHCI_HCOPS_DCBAAP, (uint64_t)(uint32_t)get_phys_addr(this_ctrlr->dcbaap), this_ctrlr);

	// Allocate space for a command ring
	this_ctrlr->cmdring = alloc_page(1); // Guaranteed not to cross 64k boundary
	memset(this_ctrlr->cmdring, 0, 4096);
	this_ctrlr->cmdring[127].param = (uint64_t)(uint32_t)get_phys_addr(this_ctrlr->cmdring);
	this_ctrlr->cmdring[127].status = 0;
	this_ctrlr->cmdring[127].command = XHCI_TRB_TRBTYPE(XHCI_TRBTYPE_LINK);
	_wr64(this_ctrlr->hcops + XHCI_HCOPS_CRCR, (uint64_t)(uint32_t)get_phys_addr(this_ctrlr->cmdring) | XHCI_HCOPS_CRCR_RCS, this_ctrlr);
	this_ctrlr->ccmdtrb = this_ctrlr->cmdring;
	this_ctrlr->cmdcycle = XHCI_HCOPS_CRCR_RCS;

	// Set up other variables
	_wr32(this_ctrlr->hcops + XHCI_HCOPS_CONFIG, _rd32(this_ctrlr->baseaddr + XHCI_HCCAP_HCSPARAM1) & 0xFF);

	_wr32(this_ctrlr->hcops + XHCI_HCOPS_DNCTRL, 2);

	// Create an event ring
	this_ctrlr->evtring_table = alloc_page(1);
	memset(this_ctrlr->evtring_table, 0, 4096);
	this_ctrlr->evtring_alloc = alloc_page(80);
	memset(this_ctrlr->evtring_alloc, 0, 4096 * 80);
	this_ctrlr->evtring = (void *)((uint32_t)(this_ctrlr->evtring_alloc + (16 * 4096)) & 0xFFFF0000);
	this_ctrlr->cevttrb = this_ctrlr->evtring;
	this_ctrlr->evtcycle = 1;

	// Set table address
	*(uint64_t *)this_ctrlr->evtring_table = (uint64_t)(uint32_t)get_phys_addr(this_ctrlr->evtring);
	*(uint32_t *)(this_ctrlr->evtring_table + 8) = 4096;

	// Assign the event ring to the primary interrupter (IR_0)
	_wr32(this_ctrlr->runtime + XHCI_RUNTIME_IR0 + XHCI_INTREG_IMR, XHCI_INTREG_IMR_IP | XHCI_INTREG_IMR_EN);
	_wr32(this_ctrlr->runtime + XHCI_RUNTIME_IR0 + XHCI_INTREG_IMOD, 0);
	_wr32(this_ctrlr->runtime + XHCI_RUNTIME_IR0 + XHCI_INTREG_ERSTS, 1);
	_wr64(this_ctrlr->runtime + XHCI_RUNTIME_IR0 + XHCI_INTREG_ERDQPTR, ((uint64_t)(uint32_t)get_phys_addr(this_ctrlr->evtring)) | XHCI_INTREG_ERDQPTR_EHBSY, this_ctrlr);
	_wr64(this_ctrlr->runtime + XHCI_RUNTIME_IR0 + XHCI_INTREG_ERSTBA, (uint64_t)(uint32_t)get_phys_addr(this_ctrlr->evtring_table), this_ctrlr);

	_wr32(this_ctrlr->hcops + XHCI_HCOPS_USBSTS, XHCI_HCOPS_USBSTS_HSE | XHCI_HCOPS_USBSTS_EINT | XHCI_HCOPS_USBSTS_PCD | XHCI_HCOPS_USBSTS_SRE);

	add_irq_function(irq, &xhci_interrupt);

	_wr32(this_ctrlr->hcops + XHCI_HCOPS_USBCMD, XHCI_HCOPS_USBCMD_RS | XHCI_HCOPS_USBCMD_INTE | XHCI_HCOPS_USBCMD_HSEE);

	for (uint8_t current_port = 0; current_port < this_ctrlr->num_ports; current_port++) {
		// Ignore any USB 2 port with a pair for now. We'll get them later.
		if (this_ctrlr->ports[current_port].legacy & XHCI_PORT_LEGACY_USB2 && this_ctrlr->ports[current_port].port_pair != 0xFF) {
			continue;
		}
		if (xhci_port_reset(this_ctrlr, current_port)) {
			dbgprintf("[xHCI] Reset port %$ (USB%c) SUCCESS\n", current_port, this_ctrlr->ports[current_port].legacy & XHCI_PORT_LEGACY_USB2 ? '2' : '3');
			xhci_init_port_dev(this_ctrlr, current_port, 0, 5);
		} else {
			dbgprintf("[xHCI] Reset port %$ (USB3) FAILED\n", current_port);
			if (this_ctrlr->ports[current_port].port_pair != 0xFF) {
				if (xhci_port_reset(this_ctrlr, this_ctrlr->ports[current_port].port_pair)) {
					dbgprintf("[xHCI] Reset port %$ (USB2) SUCCESS\n", this_ctrlr->ports[current_port].port_pair);
					xhci_init_port_dev(this_ctrlr, this_ctrlr->ports[current_port].port_pair, 0, 5);
				} else
					dbgprintf("[xHCI] Reset port %$ (USB2) FAILED\n", this_ctrlr->ports[current_port].port_pair);
			}
		}
	}

	return xhci_num_ctrlrs;
}

xhci_controller *get_xhci_controller(uint8_t id) { return &xhci_controllers[id]; }

uint8_t xhci_get_unused_device(xhci_controller *xc) {
	for (uint8_t i = 1; i; i++) {
		if (!(xc->devices[i].valid))
			return i;
	}
	return 0;
}

void xhci_advance_trb(xhci_trb *ring_start, xhci_trb **current_pointer, uint8_t *current_cycle) {
	(*current_pointer)++;

	if (XHCI_TRB_GTRBTYPE((*current_pointer)->command) == XHCI_TRBTYPE_LINK) {
		(*current_pointer)->param = (uintptr_t)get_phys_addr(ring_start);
		(*current_pointer)->status = 0;
		(*current_pointer)->command = XHCI_TRB_TRBTYPE(XHCI_TRBTYPE_LINK) | *current_cycle | XHCI_TRB_LINK_FLIPCYCLE;
		DATA("[xHCI] DATA: Send LINK TRB\n");
		DATA("[xHCI] DATA: Param: %#\n", (uint64_t)(*current_pointer)->param);
		DATA("[xHCI] DATA: Status: %#\n", (uint64_t)(*current_pointer)->status);
		DATA("[xHCI] DATA: Command: %#\n", (uint64_t)(*current_pointer)->command);
		*current_cycle ^= 1;
		DATA("[xHCI] DATA: NEW CYCLE: %$\n", (uint32_t)*current_cycle);
		*current_pointer = ring_start;
	}
}

uint32_t xhci_await_int(uint32_t *status) {
	uint16_t timeout = 2000;
	while (timeout--) {
		if (*status)
			break;
		sleep(1);
	}

	return *status;
}

xhci_trb xhci_send_cmdtrb(xhci_controller *xc, xhci_trb trb) {
	xhci_trb *this_trb = xc->ccmdtrb;
	trb.command |= xc->cmdcycle;
	*this_trb = trb;
	DATA("[xHCI] DATA: Send COMMAND TRB\n");
	DATA("[xHCI] DATA: Param: %#\n", this_trb->param);
	DATA("[xHCI] DATA: Status: %#\n", (uint64_t)this_trb->status);
	DATA("[xHCI] DATA: Command: %#\n", (uint64_t)this_trb->command);

	xhci_advance_trb(xc->cmdring, &xc->ccmdtrb, &xc->cmdcycle);

	DATA("[xHCI] DATA: New CCMDTRB: %#\n", (uint64_t)(uint32_t)xc->ccmdtrb);

	ring_doorbell(xc, 0, 0);

	uint16_t timeout = 2000;
	while (timeout--) {
		if (this_trb->param != trb.param)
			break;
		sleep(1);
	}

	xhci_trb retval;
	memset(&retval, 0, sizeof(xhci_trb));

	if (!timeout || this_trb->param == trb.param)
		return retval;

	retval = *(xhci_trb *)(uintptr_t)this_trb->param;

	return retval;

	// dbgprintf("USBSTS: %#\n",(uint64_t)_rd32(xc->hcops+XHCI_HCOPS_USBSTS));
	// dbgprintf("IR0_1: %#\n",(uint64_t)_rd32(xc->runtime+XHCI_RUNTIME_IR0+XHCI_INTREG_IMR));
	// dbgprintf("IR0_2: %#\n",_rd64(xc->runtime+XHCI_RUNTIME_IR0+XHCI_INTREG_ERDQPTR));
}

void xhci_setup_trb(usb_device *device, usb_setup_pkt setup_pkt, uint8_t direction, uint8_t endpt_num, bool chain) {
	xhci_devdata *data = device->data;

	xhci_trb trb;
	trb.param = *(uint64_t *)&setup_pkt; // Convert setup packet into uint64_t
	trb.status = 8;
	trb.command = XHCI_TRB_SETUP_DIRECTION(direction) | XHCI_TRB_TRBTYPE(XHCI_TRBTYPE_SETUP) | XHCI_TRB_IMMEDIATE | data->endpt_cycle[endpt_num] | (chain ? XHCI_TRB_CHAIN : 0);

	*data->cendpttrb[endpt_num] = trb;

	DATA("[xHCI] DATA: Send SETUP TRB\n");
	DATA("[xHCI] DATA: Param: %#\n", trb.param);
	DATA("[xHCI] DATA: Status: %#\n", (uint64_t)trb.status);
	DATA("[xHCI] DATA: Command: %#\n", (uint64_t)trb.command);

	xhci_advance_trb(data->endpt_ring[endpt_num], &data->cendpttrb[endpt_num], &data->endpt_cycle[endpt_num]);

	DATA("[xHCI] DATA: New CENDPTTRB: %#\n", (uint64_t)(uint32_t)data->cendpttrb[endpt_num]);
}

void xhci_data_trbs(usb_device *device, void *buffer, uint32_t *status, uint16_t size, uint8_t direction, uint8_t endpt_num) {
	void *        current_buffer = buffer;
	xhci_devdata *data = device->data;
	uint16_t      size_remaining = size;
	uint8_t       trbs_remaining = ((size + (device->max_pkt_size - 1)) / device->max_pkt_size) - 1;
	uint8_t       trb_type = XHCI_TRBTYPE_DATA;

	while (size_remaining) {
		xhci_trb trb;
		trb.param = (uint64_t)(uint32_t)get_phys_addr(current_buffer);
		trb.status = XHCI_TRB_DATA_REMAINING(trbs_remaining) | ((size_remaining < device->max_pkt_size) ? size_remaining : device->max_pkt_size);
		trb.command = XHCI_TRB_DATA_DIRECTION(direction) | XHCI_TRB_TRBTYPE(trb_type) | XHCI_TRB_CHAIN | (trbs_remaining ? 0 : XHCI_TRB_EVALTRB) | data->endpt_cycle[endpt_num];
		*data->cendpttrb[endpt_num] = trb;

		DATA("[xHCI] DATA: TRBs remain: %$\n", trbs_remaining);
		DATA("[xHCI] DATA: Send DATA TRB\n");
		DATA("[xHCI] DATA: Param: %#\n", trb.param);
		DATA("[xHCI] DATA: Status: %#\n", (uint64_t)trb.status);
		DATA("[xHCI] DATA: Command: %#\n", (uint64_t)trb.command);

		xhci_advance_trb(data->endpt_ring[endpt_num], &data->cendpttrb[endpt_num], &data->endpt_cycle[endpt_num]);

		DATA("[xHCI] DATA: New CENDPTTRB: %#\n", (uint64_t)(uint32_t)data->cendpttrb[endpt_num]);

		current_buffer += device->max_pkt_size;
		size_remaining -= ((size_remaining < device->max_pkt_size) ? size_remaining : device->max_pkt_size);
		trbs_remaining--;

		trb_type = XHCI_TRBTYPE_NORMAL;
		direction = 0;
	}

	*status = 0;

	xhci_trb trb;
	trb.param = (uint64_t)(uint32_t)get_phys_addr(status);
	trb.status = 0;
	trb.command = XHCI_TRB_TRBTYPE(XHCI_TRBTYPE_EVENT) | XHCI_TRB_IOC | data->endpt_cycle[endpt_num];
	*data->cendpttrb[endpt_num] = trb;

	DATA("[xHCI] DATA: Send EVENT TRB\n");
	DATA("[xHCI] DATA: Param: %#\n", trb.param);
	DATA("[xHCI] DATA: Status: %#\n", (uint64_t)trb.status);
	DATA("[xHCI] DATA: Command: %#\n", (uint64_t)trb.command);

	xhci_advance_trb(data->endpt_ring[endpt_num], &data->cendpttrb[endpt_num], &data->endpt_cycle[endpt_num]);

	DATA("[xHCI] DATA: New CENDPTTRB: %#\n", (uint64_t)(uint32_t)data->cendpttrb[endpt_num]);
}

void xhci_status_trb(usb_device *device, uint32_t *status, uint8_t direction, uint8_t endpt_num) {
	xhci_devdata *data = device->data;

	xhci_trb trb;
	trb.param = 0;
	trb.status = 0;
	trb.command = XHCI_TRB_DATA_DIRECTION(direction ^ 1) | XHCI_TRB_TRBTYPE(XHCI_TRBTYPE_STATUS) | XHCI_TRB_CHAIN | data->endpt_cycle[endpt_num];

	*data->cendpttrb[endpt_num] = trb;

	DATA("[xHCI] DATA: Send STATUS TRB\n");
	DATA("[xHCI] DATA: Param: %#\n", trb.param);
	DATA("[xHCI] DATA: Status: %#\n", (uint64_t)trb.status);
	DATA("[xHCI] DATA: Command: %#\n", (uint64_t)trb.command);

	xhci_advance_trb(data->endpt_ring[endpt_num], &data->cendpttrb[endpt_num], &data->endpt_cycle[endpt_num]);

	DATA("[xHCI] DATA: New CENDPTTRB: %#\n", (uint64_t)(uint32_t)data->cendpttrb[endpt_num]);

	*status = 0;

	trb.param = (uint64_t)(uint32_t)get_phys_addr(status);
	trb.status = 0;
	trb.command = XHCI_TRB_TRBTYPE(XHCI_TRBTYPE_EVENT) | XHCI_TRB_IOC | data->endpt_cycle[endpt_num];

	*data->cendpttrb[endpt_num] = trb;

	DATA("[xHCI] DATA: Send EVENT TRB\n");
	DATA("[xHCI] DATA: Param: %#\n", trb.param);
	DATA("[xHCI] DATA: Status: %#\n", (uint64_t)trb.status);
	DATA("[xHCI] DATA: Command: %#\n", (uint64_t)trb.command);

	xhci_advance_trb(data->endpt_ring[endpt_num], &data->cendpttrb[endpt_num], &data->endpt_cycle[endpt_num]);

	DATA("[xHCI] DATA: New CENDPTTRB: %#\n", (uint64_t)(uint32_t)data->cendpttrb[endpt_num]);
}

bool xhci_generic_setup(usb_device *device, usb_setup_pkt setup_pkt_template) {
	xhci_controller *xc = device->controller;
	void *           data = alloc_page(1);
	uint32_t *       status = data;

	xc->lock = MUTEX_STATE_LOCKED;

	xhci_setup_trb(device, setup_pkt_template, XHCI_DIRECTION_NONE, XHCI_ENDPT_CONTROL, true);
	xhci_status_trb(device, status, XHCI_DATA_DIR_IN, XHCI_ENDPT_CONTROL);
	ring_doorbell(xc, device->slot, 1);

	*status = xhci_await_int(status);

	xc->lock = MUTEX_STATE_UNLOCKED;

	DATA("[xHCI] Looking at PhysAddr %llX\n", get_phys_addr(status));
	DATA("[xHCI] Recieved status code %X\n", *status);

	if (XHCI_EVTTRB_STATUS_GETCODE(*status) == XHCI_TRBCODE_SUCCESS) {
		dbgprintf("[xHCI] Successfully sent generic setup packet.\n");
		free_page(data, 1);
		return true;
	}

	dbgprintf("[xHCI] Error: Setup Packet timed out or bad code\n");
	free_page(data, 1);
	return false;
}

// Set the address of a usb device.
bool xhci_set_address(usb_device *device, uint32_t command_params) {
	xhci_controller *xc = device->controller;
	xhci_devdata *   data = device->data;
	xhci_slot *      slot_copy = (xhci_slot *)(data->slot_template + xc->ctx_size);
	xhci_endpt *     endpt_copy = (xhci_endpt *)(data->slot_template + (xc->ctx_size * 2));
	xhci_slot *      active_slot = data->slot_ctx;
	xhci_endpt *     active_endpt = (xhci_endpt *)((void *)(data->slot_ctx) + xc->ctx_size);

	dbgdump_memory32((void *)(uintptr_t)slot_copy, xc->ctx_size / 4);

	xhci_trb trb;
	trb.param = (uintptr_t)get_phys_addr(data->slot_template);
	trb.status = 0;
	trb.command = XHCI_TRB_COMMAND_SLOT(device->slot) | XHCI_TRB_TRBTYPE(XHCI_TRBTYPE_ADDR) | command_params;

	dbgprintf("Param: %llX\n", trb.param);
	dbgprintf("Status: %X\n", trb.status);
	dbgprintf("Command: %X\n", trb.command);

	trb = xhci_send_cmdtrb(xc, trb);

	if (XHCI_EVTTRB_STATUS_GETCODE(trb.status) != XHCI_TRBCODE_SUCCESS) {
		dbgprintf("Error: SetAddress returned %X\n", trb.status);
		return false;
	}

	// dbgprintf("\n------------------------------------------------------------\n");
	dbgprintf("[xHCI] Got Device Address %$\n", active_slot->devaddr);
	slot_copy->state = active_slot->state;
	slot_copy->devaddr = active_slot->devaddr;
	endpt_copy->state = active_endpt->state;
	endpt_copy->max_pkt_size = active_endpt->max_pkt_size;

	/*dbgprintf("------------------------------------------------------------\n");
	dbgprintf("This data is pointed to by DCBAAP+Slot#%$\n",device->slot);
	dump_memory32(data->slot_ctx,256/4);
	dbgprintf("\n------------------------------------------------------------\n");
	dbgprintf("This data is pointed to by the Set Address TRB.\n");
	dump_memory32(data->slot_template,xc->ctx_size/4);
	dbgprintf("\n\n");
	dump_memory32(data->slot_template+xc->ctx_size,256/4);
	dbgprintf("\n");*/

	return true;
}

bool xhci_enable_endpoint(usb_device *device, uint8_t endpoint, uint8_t flags, uint8_t interval) {
	dbgprintf("[xHCI] Attempting to enable endpoint %u as type %u\n", endpoint, flags);
	xhci_controller *xc = (xhci_controller *)device->controller;
	xhci_devdata *   data = device->data;
	void *           slot_template = data->slot_template;
	xhci_slot *      slot_ctx = slot_template + xc->ctx_size;

	xhci_endpt ep;
	ep.max_pkt_size = device->max_pkt_size;
	ep.state = XHCI_ENDPT_STATE_STATE(XHCI_ENDPOINT_STATE_DISABLE);
	ep.flags = XHCI_ENDPT_FLAGS_ENDPT_TYPE(flags) | XHCI_ENDPT_FLAGS_ERRCNT(3);
	ep.interval = interval + 2;
	ep.avg_trblen = 8;

	uint8_t endpoint_index = (endpoint * 2) + (flags & 4 ? 1 : 0) - 1;
	dbgprintf("[xHCI] Endpoint index is %u\n", endpoint_index);

	data->endpt_ring[endpoint_index] = alloc_page(1);
	data->cendpttrb[endpoint_index] = data->endpt_ring[endpoint_index];
	ep.dqptr = (uint64_t)(uint32_t)get_phys_addr(data->endpt_ring[endpoint_index]) | XHCI_TRB_CYCLE;
	memset(data->endpt_ring[endpoint_index], 0, 4096);
	data->endpt_ring[endpoint_index][127].param = ep.dqptr;
	data->endpt_ring[endpoint_index][127].status = 0;
	data->endpt_ring[endpoint_index][127].command = XHCI_TRB_TRBTYPE(XHCI_TRBTYPE_LINK);
	data->endpt_cycle[endpoint_index] = XHCI_TRB_CYCLE;

	uint32_t *  input_context = slot_template;
	xhci_endpt *endpoints = slot_template + (xc->ctx_size * 2);
	endpoints[endpoint_index] = ep;

	uint8_t old_entries = (slot_ctx->entries >> 27) & 31;
	slot_ctx->entries &= ~XHCI_SLOT_ENTRY_COUNT(31);
	slot_ctx->entries |= XHCI_SLOT_ENTRY_COUNT(old_entries + 1);

	input_context[0] = 0;
	input_context[1] = 1 << (endpoint_index + 1) | 1;
	dbgprintf("[xHCI] Input Context Enable: %X\n", input_context[1]);

	xhci_trb trb;
	trb.param = (uintptr_t)get_phys_addr(data->slot_template);
	trb.status = 0;
	trb.command = XHCI_TRB_COMMAND_SLOT(device->slot) | XHCI_TRB_TRBTYPE(XHCI_TRBTYPE_CONFIG);

	DATA("[xHCI] DATA: Send CONFIG TRB\n");
	DATA("[xHCI] DATA: Param: %#\n", trb.param);
	DATA("[xHCI] DATA: Status: %#\n", (uint64_t)trb.status);
	DATA("[xHCI] DATA: Command: %#\n", (uint64_t)trb.command);

	trb = xhci_send_cmdtrb(xc, trb);

	DATA("[xHCI] DATA: Recieved EVENT TRB\n");
	DATA("[xHCI] DATA: Param: %#\n", trb.param);
	DATA("[xHCI] DATA: Status: %#\n", (uint64_t)trb.status);
	DATA("[xHCI] DATA: Command: %#\n", (uint64_t)trb.command);

	return XHCI_EVTTRB_STATUS_GETCODE(trb.status) == XHCI_TRBCODE_SUCCESS;
}

bool xhci_register_hub(usb_device *device) {
	dbgprintf("[xHCI] Attempting to register hub\n");
	xhci_controller *xc = (xhci_controller *)device->controller;
	xhci_devdata *   data = device->data;
	void *           slot_template = data->slot_template;
	xhci_slot *      slot_ctx = slot_template + xc->ctx_size;
	uint32_t *       input_context = slot_template;

	slot_ctx->entries |= XHCI_SLOT_ENTRY_HUB;

	input_context[0] = 0;
	input_context[1] = 1;

	dbgdump_memory32((void *)(uintptr_t)slot_ctx, xc->ctx_size / 4);

	xhci_trb trb;
	trb.param = (uintptr_t)get_phys_addr(data->slot_template);
	trb.status = 0;
	trb.command = XHCI_TRB_COMMAND_SLOT(device->slot) | XHCI_TRB_TRBTYPE(XHCI_TRBTYPE_EVAL);

	DATA("[xHCI] DATA: Send CONFIG TRB\n");
	DATA("[xHCI] DATA: Param: %#\n", trb.param);
	DATA("[xHCI] DATA: Status: %#\n", (uint64_t)trb.status);
	DATA("[xHCI] DATA: Command: %#\n", (uint64_t)trb.command);

	trb = xhci_send_cmdtrb(xc, trb);

	DATA("[xHCI] DATA: Recieved EVENT TRB\n");
	DATA("[xHCI] DATA: Param: %#\n", trb.param);
	DATA("[xHCI] DATA: Status: %#\n", (uint64_t)trb.status);
	DATA("[xHCI] DATA: Command: %#\n", (uint64_t)trb.command);

	return XHCI_EVTTRB_STATUS_GETCODE(trb.status) == XHCI_TRBCODE_SUCCESS;
}

uint32_t xhci_next_route(uint32_t routing, uint8_t port) {
	dbgprintf("[xHCI] Routing port %u through %X\n", port, routing);
	uint8_t shift = 0;
	for (uint8_t i = 0; i < 4; i++) {
		if (!(routing & (0xF << shift)))
			return routing | (port << shift);
		shift += 4;
	}
	dbgprintf("[xHCI] Failed to find available route\n");
	return 0;
}

bool xhci_assign_address(uint16_t parentaddr, uint8_t port, uint8_t speed) {
	usb_device *     parent = usb_device_from_addr(parentaddr);
	xhci_controller *xc = parent->controller;
	dbgprintf("[xHCI] Assigning address via routing: %X\n", xhci_next_route(parent->routing, port + 1));
	return xhci_init_port_dev(xc, parent->port, xhci_next_route(parent->routing, port + 1), speed);
}

bool xhci_usb_get_desc(usb_device *device, void *out, usb_setup_pkt setup_pkt_template, uint16_t size) {
	xhci_controller *xc = device->controller;
	void *           data = alloc_page(((size + sizeof(uint32_t)) / 4096) + 1);
	uint32_t *       status = data;
	void *           buffer = data + 16;

	xc->lock = MUTEX_STATE_LOCKED;

	xhci_setup_trb(device, setup_pkt_template, XHCI_DIRECTION_IN, XHCI_ENDPT_CONTROL, false);
	xhci_data_trbs(device, buffer, status, size, XHCI_DATA_DIR_IN, XHCI_ENDPT_CONTROL);
	xhci_status_trb(device, status, XHCI_DATA_DIR_IN, XHCI_ENDPT_CONTROL);
	ring_doorbell(xc, device->slot, 1);

	*status = xhci_await_int(status);

	xc->lock = MUTEX_STATE_UNLOCKED;

	DATA("[xHCI] Looking at PhysAddr %#\n", (uint64_t)(uint32_t)get_phys_addr(status));
	DATA("[xHCI] Recieved status code %#\n", (uint64_t)*status);

	if (XHCI_EVTTRB_STATUS_GETCODE(*status) == XHCI_TRBCODE_SUCCESS) {
		memcpy(out, buffer, size);
		dbgdump_memory(buffer, size);
		dbgprintf("\n");
		return true;
	}

	dbgprintf("[xHCI] Error: Device Descriptor timed out or bad code: %X%s\n", XHCI_EVTTRB_STATUS_GETCODE(*status), XHCI_EVTTRB_STATUS_GETCODE(*status) ? "" : " (timeout detected)");
	return false;
}

void *xhci_create_interval_in(usb_device *device, void *out, uint8_t interval, uint8_t endpoint_addr, uint16_t max_pkt_size, uint16_t size) {
	(void)interval;
	(void)max_pkt_size;
	uint8_t endpoint_index = endpoint_addr * 2; // We know this must be an input, since the interval is only for input

	xhci_controller *xc = (xhci_controller *)device->controller;
	xhci_devdata *   data = device->data;

	xhci_trb trb;
	trb.param = (uint64_t)(uint32_t)get_phys_addr(out);
	trb.status = size;
	trb.command = XHCI_TRB_TRBTYPE(XHCI_TRBTYPE_NORMAL) | XHCI_TRB_IOC | XHCI_TRB_CYCLE;

	for (uint8_t i = 0; i < 127; i++) {
		data->endpt_ring[endpoint_index][i] = trb;
	}
	data->endpt_ring[endpoint_index][128] = trb;

	data->endpt_ring[endpoint_index][127].param = (uint64_t)(uint32_t)get_phys_addr(data->endpt_ring[endpoint_index]);
	data->endpt_ring[endpoint_index][127].status = 0;
	data->endpt_ring[endpoint_index][127].command = XHCI_TRB_TRBTYPE(XHCI_TRBTYPE_LINK) | XHCI_TRB_LINK_FLIPCYCLE | XHCI_TRB_CYCLE;

	data->endpt_cycle[endpoint_index] = XHCI_TRB_CYCLE;

	ring_doorbell(xc, device->slot, endpoint_index);

	return data->endpt_ring[endpoint_index];
}

bool xhci_refresh_interval(void *data) {
	if (!data)
		return false;
	xhci_trb *trbs = data;
	bool      rval = false;

	for (uint8_t i = 0; i < 127; i++) {
		if (trbs[i].param != trbs[128].param) {
			trbs[i].param = trbs[128].param;
			trbs[i].status = trbs[128].status;
			trbs[i].command ^= XHCI_TRB_CYCLE;
			rval = true;
		}
		if (!i)
			trbs[127].command ^= XHCI_TRB_CYCLE;
	}

	return rval;
}

void xhci_destroy_interval(void *data) {
	xhci_trb *ring = data;

	xhci_trb trb;
	trb.param = 0;
	trb.status = 0;
	trb.command = XHCI_TRB_TRBTYPE(XHCI_TRBTYPE_NOOP);

	for (uint8_t i = 0; i < 127; i++) {
		ring[i] = trb;
	}
}

usb_dev_desc xhci_get_usb_dev_descriptor(usb_device *device, uint16_t size) {
	usb_dev_desc retval;
	memset(&retval, 0, sizeof(usb_dev_desc));

	// Generate a setup packet
	usb_setup_pkt setup_pkt_template = {0};
	setup_pkt_template.type = 0x80;
	setup_pkt_template.request = 6;
	setup_pkt_template.value = 0x100;
	setup_pkt_template.index = 0;
	setup_pkt_template.length = size;

	if (xhci_usb_get_desc(device, &retval, setup_pkt_template, size))
		dbgprintf("[xHCI] Successfully got USB device descriptor.\n");
	else
		dbgprintf("[xHCI] Failed to get USB Device descriptor\n");

	return retval;
}

bool xhci_get_usb_str_desc(usb_device *device, char *out, uint8_t index, uint16_t targetlang) {
	usb_desc_header header;
	usb_setup_pkt   sp;
	sp.type = 0x80;
	sp.request = 6;
	sp.value = 0x300;
	sp.index = 0;
	sp.length = 2;
	if (!xhci_usb_get_desc(device, &header, sp, 2))
		return false;
	sp.length = header.length;
	char buffer[256];
	memset(buffer, 0, 256);
	usb_str_desc *strdesc = (usb_str_desc *)buffer;
	if (!xhci_usb_get_desc(device, buffer, sp, header.length))
		return false;
	uint8_t i;
	for (i = 0; i < (strdesc->length - 2) / 2; i++) {
		if (strdesc->langid[i] == targetlang)
			break;
	}
	if (i == (strdesc->length - 2) / 2)
		return false;

	sp.index = strdesc->langid[i];
	sp.value = 0x300 | index;
	printf("Getting index %u, (0x%X)\n", index, sp.value);
	sp.length = 2;
	memset(buffer, 0, 256);
	if (!xhci_usb_get_desc(device, &header, sp, 2))
		return false;
	sp.length = header.length;
	if (!xhci_usb_get_desc(device, buffer, sp, header.length))
		return false;
	uint16_t k = 0;
	for (uint16_t j = 2; j < header.length; j++)
		out[k++] = buffer[j++];

	dbgprintf("[xHCI] Successfully got USB string descriptor.\n");
	return true;
}

usb_config_desc xhci_get_config_desc(usb_device *device, uint8_t index) {
	usb_desc_header header;
	usb_setup_pkt   sp;
	memset(&sp, 0, sizeof(usb_setup_pkt));
	usb_config_desc config;
	memset(&config, 0, sizeof(usb_config_desc));
	sp.type = 0x80;
	sp.request = 6;
	sp.value = 0x200 | index;
	sp.index = 0;
	sp.length = 2;
	if (!xhci_usb_get_desc(device, &header, sp, 2))
		return config;
	sp.length = header.length;
	if (!xhci_usb_get_desc(device, &config, sp, header.length))
		return config;

	dbgprintf("[xHCI] Successfully got USB config descriptor\n");
	return config;
}

void *xhci_get_next_desc(uint8_t type, void *start, void *end) {
	void *bufferptr = start;
	while (bufferptr < end) {
		usb_desc_header *header = bufferptr;
		if (header->type == type)
			break;
		if (header->length)
			bufferptr += header->length;
		else
			bufferptr = end;
	}
	if (bufferptr >= end) {
		return 0;
	} else
		return bufferptr;
}

usb_interface_desc xhci_get_interface_desc(usb_device *device, uint8_t config_index, uint8_t interface_index) {
	usb_config_desc    config = xhci_get_config_desc(device, config_index);
	usb_interface_desc interface;
	memset(&interface, 0, sizeof(usb_interface_desc));

	if (config.num_interfaces < interface_index)
		return interface;

	usb_setup_pkt sp;
	sp.type = 0x80;
	sp.request = 6;
	sp.value = 0x200 | config_index;
	sp.index = 0;
	sp.length = config.total_len;
	uint16_t num_pages = (config.total_len / 4096) + 1;
	void *   buffer = alloc_page(num_pages);
	void *   buffer_end = buffer + (num_pages * 4096);
	memset(buffer, 0, num_pages * 4096);
	if (!xhci_usb_get_desc(device, buffer, sp, config.total_len)) {
		free_page(buffer, num_pages);
		return interface;
	}
	void *bufferptr = buffer + sizeof(usb_config_desc);
	for (uint8_t i = 0; i < interface_index; i++) {
		bufferptr = xhci_get_next_desc(USB_DESC_INTERFACE, bufferptr, buffer_end);
		if (!bufferptr) {
			free_page(buffer, num_pages);
			return interface;
		}
	}
	bufferptr = xhci_get_next_desc(USB_DESC_INTERFACE, bufferptr, buffer_end);
	if (!bufferptr) {
		free_page(buffer, num_pages);
		return interface;
	}

	interface = *(usb_interface_desc *)bufferptr;

	free_page(buffer, num_pages);
	dbgprintf("[xHCI] Successfully got USB interface descriptor\n");
	return interface;
}

usb_endpoint_desc xhci_get_endpoint_desc(usb_device *device, uint8_t config_index, uint8_t interface_index, uint8_t endpoint_index) {
	usb_config_desc   config = xhci_get_config_desc(device, config_index);
	usb_endpoint_desc endpoint;
	memset(&endpoint, 0, sizeof(usb_endpoint_desc));

	if (config.num_interfaces < interface_index)
		return endpoint;

	usb_setup_pkt sp;
	sp.type = 0x80;
	sp.request = 6;
	sp.value = 0x200 | config_index;
	sp.index = 0;
	sp.length = config.total_len;
	uint16_t num_pages = (config.total_len / 4096) + 1;
	void *   buffer = alloc_page(num_pages);
	void *   buffer_end = buffer + (num_pages * 4096);
	memset(buffer, 0, num_pages * 4096);
	if (!xhci_usb_get_desc(device, buffer, sp, config.total_len)) {
		free_page(buffer, num_pages);
		return endpoint;
	}
	dbgdump_memory(buffer, config.total_len);
	void *bufferptr = buffer + sizeof(usb_config_desc);
	for (uint8_t i = 0; i < interface_index; i++) {
		bufferptr = xhci_get_next_desc(USB_DESC_INTERFACE, bufferptr, buffer_end);
		if (!bufferptr) {
			free_page(buffer, num_pages);
			return endpoint;
		}
	}
	bufferptr = xhci_get_next_desc(USB_DESC_INTERFACE, bufferptr, buffer_end);
	if (!bufferptr) {
		free_page(buffer, num_pages);
		return endpoint;
	}
	for (uint8_t i = 0; i < endpoint_index; i++) {
		bufferptr = xhci_get_next_desc(USB_DESC_ENDPOINT, bufferptr, buffer_end);
		if (!bufferptr) {
			free_page(buffer, num_pages);
			return endpoint;
		}
	}
	bufferptr = xhci_get_next_desc(USB_DESC_ENDPOINT, bufferptr, buffer_end);
	if (!bufferptr) {
		free_page(buffer, num_pages);
		return endpoint;
	}

	endpoint = *(usb_endpoint_desc *)bufferptr;

	dbgprintf("[xHCI] Endpoint: %#\n", (uint64_t)(uint32_t)(bufferptr));

	free_page(buffer, num_pages);
	dbgprintf("[xHCI] Successfully got USB endpoint descriptor\n");
	return endpoint;
}
