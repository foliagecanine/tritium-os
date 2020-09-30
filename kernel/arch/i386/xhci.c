#include <usb/xhci.h>

#define DEBUG

#ifdef DEBUG
#ifdef DEBUG_TERMINAL
#define dbgprintf printf
#else
#define dbgprintf dprintf
#endif
#else
#define dbgprintf
#endif

xhci_controller xhci_controllers[USB_MAX_CTRLRS];
uint8_t xhci_num_ctrlrs = 0;

// Force a 64 bit read
inline volatile uint64_t _rd64(volatile void *mem) {
	return *(volatile uint64_t *)mem;
}

// Force a 32 bit read
inline volatile uint32_t _rd32(volatile void *mem) {
	return *(volatile uint32_t *)mem;
}

// Force a 16 bit read
inline volatile uint16_t _rd16(volatile void *mem) {
	return (uint16_t)((*(volatile uint32_t *)(void *)((uint32_t)mem&~3))>>(((uint32_t)mem&3)*8));
}

// Force an 8 bit read
inline volatile uint8_t _rd8(volatile void *mem) {
	return (uint8_t)((*(volatile uint32_t *)(void *)((uint32_t)mem&~3))>>(((uint32_t)mem&3)*8));
}

// Emulate a 64 bit write
inline void _wr64(void *mem, uint64_t b, xhci_controller *xc) {
	_wr32(mem,(uint32_t)b);
	if (xc->params&1)
		_wr32(mem+4,(uint32_t)(b>>32));
}

// Force a 32 bit write
inline void _wr32(void *mem, uint32_t b) {
	__asm__ __volatile__("movl %%eax, %0":"=m"(*mem):"a"(b):"memory");
}

bool xhci_global_reset(xhci_controller *xc) {
	// Clear the RunStop bit and wait until the controller stops
	_wr32(xc->hcops+XHCI_HCOPS_USBCMD,_rd32(xc->hcops+XHCI_HCOPS_USBCMD)&~XHCI_HCOPS_USBCMD_RS);
	uint16_t timeout = 20;
	while (!(_rd32(xc->hcops+XHCI_HCOPS_USBSTS)&XHCI_HCOPS_USBSTS_HCHALT)) {
		sleep(1);
		if (!--timeout)
			return false;
	}
	
	// Set the HCReset bit and wait until it clears itself
	timeout = 100;
	_wr32(xc->hcops+XHCI_HCOPS_USBCMD,XHCI_HCOPS_USBCMD_HCRST);
	while (_rd32(xc->hcops+XHCI_HCOPS_USBCMD)&XHCI_HCOPS_USBCMD_HCRST || _rd32(xc->hcops+XHCI_HCOPS_USBSTS)&XHCI_HCOPS_USBSTS_CNR) {
		sleep(1);
		if (!--timeout)
			return false;
	}
	dbgprintf("[xHCI] Reset xHCI Controller Globally\n");
	sleep(50);
	
	uint32_t paramoff = _rd16(xc->baseaddr+XHCI_HCCAP_HCCPARAM1+2)*4;
	uint32_t paramval = _rd32(xc->baseaddr+paramoff);
	while ((paramval&0xFF)&&(paramval&0xFF)!=1) {
		if ((paramval>>8)&0xFF) {
			paramoff += ((paramval>>8)&0xFF)*4;
			paramval = _rd32(xc->baseaddr+paramoff);
		} else {
			paramval = 0;
		}
	}
	if (paramval&0xFF) {
		_wr32(xc->baseaddr+paramoff,_rd32(xc->baseaddr+paramoff) | XHCI_LEGACY_OWNED);
		timeout = 10;
		while ((_rd32(xc->baseaddr+paramoff)&XHCI_LEGACY_MASK)!=XHCI_LEGACY_OWNED) {
			sleep(1);
			if (!--timeout)
				return false;
		}
		dbgprintf("[xHCI] BIOS Ownership released\n");
	} else {
		dbgprintf("[xHCI] No BIOS Ownership detected\n");
	}
	
	return true;
}

void xhci_pair_ports(xhci_controller *xc) {
	for (uint8_t i = 0; i < 16; i++) {
		xc->ports[i].port_pair = 0xFF;
		xc->ports[i].phys_portID = 0xFF;
	}
	
	uint32_t paramoff = _rd16(xc->baseaddr+XHCI_HCCAP_HCCPARAM1+2)*4;
	uint32_t paramval = _rd32(xc->baseaddr+paramoff);
	 
	while (paramoff) {
		if ((paramval&0xFF)==2) {
			if (_rd8(xc->baseaddr+paramoff+3)==2) {
				uint8_t count = _rd8(xc->baseaddr+paramoff+9);
				for (uint8_t i = 0; i < count; i++) {
					xc->ports[_rd8(xc->baseaddr+paramoff+8)+i-1].phys_portID = xc->num_ports_2++;
					xc->ports[_rd8(xc->baseaddr+paramoff+8)+i-1].legacy |= XHCI_PORT_LEGACY_USB2;
					if (_rd16(xc->baseaddr+paramoff+10)&2)
						xc->ports[_rd8(xc->baseaddr+paramoff+8)+i-1].legacy |= XHCI_PORT_LEGACY_HSO;
				}
			} else if (_rd8(xc->baseaddr+paramoff+3)==3) {
				uint8_t count = _rd8(xc->baseaddr+paramoff+9);
				for (uint8_t i = 0; i < count; i++) {
					xc->ports[_rd8(xc->baseaddr+paramoff+8)+i-1].phys_portID = xc->num_ports_3++;
				}
			}
		}
		paramoff += ((paramval>>8)&0xFF)*4;
		if (paramval&0xFF00)
			paramval = _rd32(xc->baseaddr+paramoff);
		else
			paramoff = 0;
	}
	
	dbgprintf("[xHCI] Found %d USB 2 ports and %d USB 3 ports.\n",xc->num_ports_2,xc->num_ports_3);
	
	for (uint8_t i = 0; i < 16; i++) {
		if (xc->ports[i].phys_portID!=0xFF) {
			for (uint8_t j = 0; j < 16; j++) {
				if (xc->ports[j].phys_portID!=0xFF) {
					if (i!=j) {
						if (xc->ports[i].phys_portID==xc->ports[j].phys_portID) {
							xc->ports[i].port_pair = j;
							xc->ports[i].legacy |= XHCI_PORT_LEGACY_PAIRED;
							xc->ports[j].port_pair = i;
							xc->ports[j].legacy |= XHCI_PORT_LEGACY_PAIRED;
						}
					}
				}
			}
			uint8_t legacy = xc->ports[i].legacy;
			dbgprintf("[xHCI] Port %d: %s%s%s, Phys %d, PortPair %d\n", (uint32_t)i,legacy&1?"USB 2 ":"USB 3 ",legacy&2?"HSO ":"",legacy&4?"Paired":"",(uint32_t)xc->ports[i].phys_portID,(uint32_t)xc->ports[i].port_pair);
		}
	}
}

bool xhci_port_reset(xhci_controller *xc, uint8_t port) {
	void *portbase = xc->hcops+XHCI_HCOPS_PORTREGS+(port*16);
	uint32_t writeflags = 0;
	
	// Give the port power
	if (!(_rd32(portbase+XHCI_PORTREGS_PORTSC)&XHCI_PORTREGS_PORTSC_PORTPWR)) {
		_wr32(portbase+XHCI_PORTREGS_PORTSC,XHCI_PORTREGS_PORTSC_PORTPWR);
		sleep(20);
		if (!(_rd32(portbase+XHCI_PORTREGS_PORTSC)&XHCI_PORTREGS_PORTSC_PORTPWR)) {
			//dbgprintf("[xHCI] Failed to init power on port %d\n",port);
			return false;
		}
		writeflags |= XHCI_PORTREGS_PORTSC_PORTPWR;
	}
	
	// Clear the status bits (but keep the power bit on)
	_wr32(portbase+XHCI_PORTREGS_PORTSC,writeflags | XHCI_PORTREGS_PORTSC_STATUS);
	
	// Do regular reset with USB 2 or a warm reset with USB 3
	if (xc->ports[port].legacy&XHCI_PORT_LEGACY_USB2) {
		_wr32(portbase+XHCI_PORTREGS_PORTSC,writeflags | XHCI_PORTREGS_PORTSC_PORTRST);
	} else {
		_wr32(portbase+XHCI_PORTREGS_PORTSC,writeflags | XHCI_PORTREGS_PORTSC_WARMRST);
	}
	
	uint16_t timeout = 500;
	while (!(_rd32(portbase+XHCI_PORTREGS_PORTSC)&XHCI_PORTREGS_PORTSC_PRSTCHG)) {
		sleep(1);
		if (!timeout--) {
			//dbgprintf("[xHCI] No port reset change on %d\n",port);
			return false;
		}
	}
	
	sleep(3);
	
	if (_rd32(portbase+XHCI_PORTREGS_PORTSC)&XHCI_PORTREGS_PORTSC_PE) {
		_wr32(portbase+XHCI_PORTREGS_PORTSC,writeflags | XHCI_PORTREGS_PORTSC_STATUS);
		return true;
	}
	
	//dprintf("[xHCI] Port %d not enabled.\n",port);
	return false;
}

void xhci_interrupt() {
	dbgprintf("[xHCI] USB INTERRUPT\n");
	for (uint8_t i = 0; i < USB_MAX_CTRLRS; i++) {
		xhci_controller *xc = get_xhci_controller(i);
		if (!xc->hcops)
			continue;
		if (!_rd32(xc->hcops+XHCI_HCOPS_USBSTS))
			continue;
		_wr32(xc->hcops+XHCI_HCOPS_USBSTS,_rd32(xc->hcops+XHCI_HCOPS_USBSTS));
		if (_rd32(xc->runtime+XHCI_RUNTIME_IR0+XHCI_INTREG_IMR)&(XHCI_INTREG_IMR_EN|XHCI_INTREG_IMR_IP)==(XHCI_INTREG_IMR_EN|XHCI_INTREG_IMR_IP)) {
			_wr32(xc->runtime+XHCI_RUNTIME_IR0+XHCI_INTREG_IMR,_rd32(xc->runtime+XHCI_RUNTIME_IR0+XHCI_INTREG_IMR));
			while (xc->cevttrb->command&XHCI_TRB_COMMAND_CYCLE==xc->evtcycle) {
				if (xc->cevttrb->command != 0x8801) {
					xhci_trb *exec_trb = (xc->cevttrb->param&0xFFF)+((void *)xc->cmdring);
					exec_trb->param = (uint64_t)(uint32_t)(void *)xc->cevttrb;
				}
				dbgprintf("Processed TRB\n");
				dbgprintf("Param: %#\n",xc->cevttrb->param);
				dbgprintf("Status: %#\n",(uint64_t)xc->cevttrb->status);
				dbgprintf("Command: %#\n",(uint64_t)xc->cevttrb->command);
				xc->cevttrb++;
			}
			dbgprintf("New ERDQPTR: %#\n",(uint64_t)(uint32_t)xc->cevttrb);
			_wr64(xc->runtime+XHCI_RUNTIME_IR0+XHCI_INTREG_ERDQPTR,((uint64_t)(uint32_t)get_phys_addr(xc->cevttrb))|XHCI_INTREG_ERDQPTR_EHBSY,xc);
		}
	}
}

void *xhci_init_slot(usb_device *usbdev, xhci_controller *xc) {
	usbdev->data0 = alloc_page(1);
	memset(usbdev->data0,0,4096);
	xhci_slot *this_slot = usbdev->data0;
	*(uint64_t *)(xc->dcbaap+(usbdev->address*8)) = (uint64_t)(uint32_t)(void *)this_slot;
	
	xhci_slot slot;
	slot.entries = XHCI_SLOT_ENTRY_COUNT(1) | XHCI_SLOT_ENTRY_SPEED(usbdev->speed);
	slot.rh_port_num = usbdev->port+1;
	slot.max_exit_latency = 0;
	slot.int_target = 0; //IR_0
	slot.state = XHCI_SLOT_STATE_D_E;
	slot.devaddr = 0;
	
	*this_slot = slot;
	
	xhci_endpt ep;
	memset(&ep,0,sizeof(xhci_endpt));
	
	ep.max_pkt_size = usbdev->max_pkt_size;
	ep.state = XHCI_ENDPT_STATE_STATE(XHCI_ENDPOINT_STATE_DISABLE);
	ep.flags = XHCI_ENDPT_FLAGS_ENDPT_TYPE(4) | XHCI_ENDPT_FLAGS_ERRCNT(3);
	ep.avg_trblen = 8;
	ep.interval = 0;
	
	ep.dqptr = alloc_page(1);
	memset((void *)ep.dqptr,0,4096);
	((xhci_trb *)ep.dqptr)[127].param = (uint64_t)(uint32_t)get_phys_addr(ep.dqptr);
	((xhci_trb *)ep.dqptr)[127].status = 0;
	((xhci_trb *)ep.dqptr)[127].command = XHCI_TRB_COMMAND_TRBTYPE(XHCI_TRBTYPE_LINK) | XHCI_TRB_COMMAND_CYCLE;
	
	xhci_endpt *this_ep = usbdev->data0+xc->ctx_size;
	*this_ep = ep;
	
	return usbdev->data0;
}

const uint16_t default_speeds[] = {64,8,64,512};

bool xhci_init_port_dev(xhci_controller *xc, uint8_t port) {
	void *portbase = xc->hcops+XHCI_HCOPS_PORTREGS+(port*16);
	uint8_t devaddr = xhci_get_unused_device(xc);
	xhci_trb trb;
	trb.param = 0;
	trb.status = 0;
	trb.command = XHCI_TRB_COMMAND_TRBTYPE(XHCI_TRBTYPE_ENSLOT);
	
	trb = xhci_send_cmdtrb(xc,trb);
	if (XHCI_EVTTRB_STATUS_GETCODE(trb.status)!=XHCI_TRBCODE_SUCCESS)
		return false;
	
	usb_device *usbdev = &xc->devices[XHCI_EVTTRB_COMMAND_GETSLOT(trb.command)];
	memset(usbdev,0,sizeof(usb_device));
	usbdev->speed = XHCI_PORTREGS_PORTSC_PORTSPD(_rd32(portbase+XHCI_PORTREGS_PORTSC));
	usbdev->controller = xc;
	usbdev->ctrlrID = xc->ctrlrID;
	usbdev->ctrlr_type = USB_CTRLR_XHCI;
	usbdev->address = XHCI_EVTTRB_COMMAND_GETSLOT(trb.command);
	usbdev->max_pkt_size = default_speeds[usbdev->speed];
	
	xhci_init_slot(usbdev,xc);
	xhci_set_address(usbdev);
}

uint8_t init_xhci_ctrlr(uint32_t baseaddr, uint8_t irq) {
	dbgprintf("[xHCI] Base Address: %#\n",(uint64_t)baseaddr);
	for (uint8_t i = 0; i < 16; i++) {
		identity_map((void *)baseaddr+(i*4096));
	}
	
	// Populate generic addresses
	xhci_controller *this_ctrlr = get_xhci_controller(xhci_num_ctrlrs++);
	this_ctrlr->ctrlrID = xhci_num_ctrlrs-1;
	this_ctrlr->baseaddr = (void *)baseaddr;
	this_ctrlr->hcops = (void *)baseaddr+_rd8(this_ctrlr->baseaddr+XHCI_HCCAP_CAPLEN);
	this_ctrlr->runtime = (void *)baseaddr+(_rd32(baseaddr+XHCI_HCCAP_RTSOFF)&XHCI_RTSOFF_RTSOFF);
	this_ctrlr->dboff = (void *)baseaddr+(_rd32(baseaddr+XHCI_HCCAP_DBOFF)&XHCI_DBOFF_DBOFF);
	this_ctrlr->params = _rd32(this_ctrlr->baseaddr+XHCI_HCCAP_HCCPARAM1);
	this_ctrlr->ctx_size = (this_ctrlr->params&4)?64:32;
	dbgprintf("[xHCI] Ops Address: %#\n",(uint64_t)(uint32_t)this_ctrlr->hcops);
	
	// Check to make sure this is a valid controller
	uint16_t hcver = _rd16(this_ctrlr->baseaddr+XHCI_HCCAP_HCIVER);
	if (hcver<0x95)
		return false;
	else
		dbgprintf("[xHCI] Controller version: %#\n",(uint64_t)hcver);
	
	// Reset xHCI controller
	if (!xhci_global_reset(this_ctrlr))
		return false;
	
	// Count total ports
	this_ctrlr->num_ports = _rd32(this_ctrlr->baseaddr+XHCI_HCCAP_HCSPARAM1)>>24;
	dbgprintf("[xHCI] Detected %d ports\n",this_ctrlr->num_ports);
	
	// Pair each USB 3 port with their USB 2 port
	xhci_pair_ports(this_ctrlr);
	
	// Allocate space for the device context base address array
	this_ctrlr->dcbaap = alloc_page(1);
	memset(this_ctrlr->dcbaap,0,4096);
	_wr64(this_ctrlr->hcops+XHCI_HCOPS_DCBAAP,(uint64_t)(uint32_t)get_phys_addr(this_ctrlr->dcbaap),this_ctrlr);
	
	// Allocate space for a command ring
	this_ctrlr->cmdring = alloc_page(1); // Guaranteed not to cross 64k boundary
	memset(this_ctrlr->cmdring,0,4096);
	this_ctrlr->cmdring[127].param = (uint64_t)(uint32_t)get_phys_addr(this_ctrlr->cmdring);
	this_ctrlr->cmdring[127].status = 0;
	this_ctrlr->cmdring[127].command = XHCI_TRB_COMMAND_TRBTYPE(XHCI_TRBTYPE_LINK) | XHCI_TRB_COMMAND_CYCLE;
	_wr64(this_ctrlr->hcops+XHCI_HCOPS_CRCR,(uint64_t)(uint32_t)get_phys_addr(this_ctrlr->cmdring)|XHCI_HCOPS_CRCR_RCS,this_ctrlr);
	this_ctrlr->ccmdtrb = this_ctrlr->cmdring;
	this_ctrlr->cmdcycle = XHCI_HCOPS_CRCR_RCS;
	
	// Set up other variables
	_wr32(this_ctrlr->hcops+XHCI_HCOPS_CONFIG,_rd32(this_ctrlr->baseaddr+XHCI_HCCAP_HCSPARAM1)&0xFF);
	
	_wr32(this_ctrlr->hcops+XHCI_HCOPS_DNCTRL,2);
	
	// Create an event ring
	this_ctrlr->evtring_table = alloc_page(1);
	memset(this_ctrlr->evtring_table,0,4096);
	this_ctrlr->evtring_alloc = alloc_page(80);
	memset(this_ctrlr->evtring_alloc,0,4096*80);
	this_ctrlr->evtring = (void *)((uint32_t)(this_ctrlr->evtring_alloc+(16*4096))&0xFFFF0000);
	this_ctrlr->cevttrb = this_ctrlr->evtring;
	this_ctrlr->evtcycle = 1;
	
	// Set table address
	*(uint64_t *)this_ctrlr->evtring_table = (uint64_t)(uint32_t)get_phys_addr(this_ctrlr->evtring);
	*(uint32_t *)(this_ctrlr->evtring_table+8) = 4096;
	
	// Assign the event ring to the primary interrupter (IR_0)
	_wr32(this_ctrlr->runtime+XHCI_RUNTIME_IR0+XHCI_INTREG_IMR, XHCI_INTREG_IMR_IP | XHCI_INTREG_IMR_EN);
	_wr32(this_ctrlr->runtime+XHCI_RUNTIME_IR0+XHCI_INTREG_IMOD, 0);
	_wr32(this_ctrlr->runtime+XHCI_RUNTIME_IR0+XHCI_INTREG_ERSTS, 1);
	_wr64(this_ctrlr->runtime+XHCI_RUNTIME_IR0+XHCI_INTREG_ERDQPTR, ((uint64_t)(uint32_t)get_phys_addr(this_ctrlr->evtring)) | XHCI_INTREG_ERDQPTR_EHBSY, this_ctrlr);
	_wr64(this_ctrlr->runtime+XHCI_RUNTIME_IR0+XHCI_INTREG_ERSTBA, (uint64_t)(uint32_t)get_phys_addr(this_ctrlr->evtring_table), this_ctrlr);
	
	_wr32(this_ctrlr->hcops+XHCI_HCOPS_USBSTS, XHCI_HCOPS_USBSTS_HSE | XHCI_HCOPS_USBSTS_EINT | XHCI_HCOPS_USBSTS_PCD | XHCI_HCOPS_USBSTS_SRE);
	
	add_irq_function(irq, &xhci_interrupt);
	
	_wr32(this_ctrlr->hcops+XHCI_HCOPS_USBCMD, XHCI_HCOPS_USBCMD_RS | XHCI_HCOPS_USBCMD_INTE | XHCI_HCOPS_USBCMD_HSEE);
	
	for (uint8_t current_port = 0; current_port < this_ctrlr->num_ports; current_port++) {
		// Ignore any USB 2 port with a pair for now. We'll get them later.
		if (this_ctrlr->ports[current_port].legacy&XHCI_PORT_LEGACY_USB2&&this_ctrlr->ports[current_port].port_pair!=0xFF) {
			continue;
		}
		if (xhci_port_reset(this_ctrlr,current_port)) {
			dbgprintf("[xHCI] Reset port %d (USB%c) SUCCESS\n",current_port,this_ctrlr->ports[current_port].legacy&XHCI_PORT_LEGACY_USB2?'2':'3');
			xhci_init_port_dev(this_ctrlr,current_port);
		} else {
			dbgprintf("[xHCI] Reset port %d (USB3) FAILED\n",current_port);
			if (this_ctrlr->ports[current_port].port_pair!=0xFF) {
				if (xhci_port_reset(this_ctrlr,this_ctrlr->ports[current_port].port_pair)) {
					dbgprintf("[xHCI] Reset port %d (USB2) SUCCESS\n",this_ctrlr->ports[current_port].port_pair);
					xhci_init_port_dev(this_ctrlr,current_port);
				} else
					dbgprintf("[xHCI] Reset port %d (USB2) FAILED\n",this_ctrlr->ports[current_port].port_pair);
			}
		}
	}
	
	return xhci_num_ctrlrs;
}

xhci_controller *get_xhci_controller(uint8_t id) {
	return &xhci_controllers[id];
}

uint8_t xhci_get_unused_device(xhci_controller *xc) {
	for (uint8_t i = 1; i; i++) {
		if (!(xc->devices[i].valid))
			return i;
	}
	return 0;
}

xhci_trb xhci_send_cmdtrb(xhci_controller *xc, xhci_trb trb) {
	xhci_trb *this_trb = xc->ccmdtrb;
	trb.command |= xc->cmdcycle;
	*this_trb = trb;
	dbgprintf("Send TRB\n");
	dbgprintf("Param: %#\n",this_trb->param);
	dbgprintf("Status: %#\n",(uint64_t)this_trb->status);
	dbgprintf("Command: %#\n",(uint64_t)this_trb->command);
	xc->ccmdtrb++;
	
	if (XHCI_TRB_COMMAND_GTRBTYPE(xc->ccmdtrb->command)==XHCI_TRBTYPE_LINK) {
		xc->ccmdtrb->command = (xc->ccmdtrb->command & ~1) | xc->cmdcycle;
		xc->cmdcycle ^= 1;
		xc->ccmdtrb = xc->cmdring;
	}
	
	dbgprintf("New CCMDTRB: %#\n",(uint64_t)(uint32_t)xc->ccmdtrb);
	
	_wr32(xc->dboff, 0);
	
	uint16_t timeout = 2000;
	while (timeout--) {
		if (this_trb->param!=trb.param)
			break;
		sleep(1);
	}
	
	xhci_trb retval;
	memset(&retval,0,sizeof(xhci_trb));
	
	if (!timeout)
		return retval;
	
	retval = *(xhci_trb *)this_trb->param;
	
	return retval;
	
	// dbgprintf("USBSTS: %#\n",(uint64_t)_rd32(xc->hcops+XHCI_HCOPS_USBSTS));
	// dbgprintf("IR0_1: %#\n",(uint64_t)_rd32(xc->runtime+XHCI_RUNTIME_IR0+XHCI_INTREG_IMR));
	// dbgprintf("IR0_2: %#\n",_rd64(xc->runtime+XHCI_RUNTIME_IR0+XHCI_INTREG_ERDQPTR));
}

bool xhci_generic_setup(usb_device *device, usb_setup_pkt setup_pkt_template) {
	
}

// Set the address of a usb device.
bool xhci_set_address(usb_device *device) {
	xhci_controller *xc = device->controller;
	device->data1 = alloc_page(1);
	memset(device->data1,0,4096);
	memcpy(device->data1+xc->ctx_size,device->data0,2048);
	((uint32_t *)device->data1)[1] = 3;
	
	xhci_trb trb;
	trb.param = get_phys_addr(device->data1);
	trb.status = 0;
	trb.command = XHCI_TRB_COMMAND_SLOT(device->address) | XHCI_TRB_COMMAND_TRBTYPE(XHCI_TRBTYPE_ADDR) | XHCI_TRB_COMMAND_BLOCK;
	trb = xhci_send_cmdtrb(xc,trb);
	dbgprintf("Device Address %d\n",((xhci_slot *)(device->data0))->devaddr);
}

bool xhci_assign_address(uint8_t ctrlrID, uint8_t port) {
	xhci_controller *xc = get_xhci_controller(ctrlrID);
	uint8_t dev_addr = xhci_get_unused_device(xc);
	usb_device usbdev;
	memset(&usbdev,0,sizeof(usb_device));
	usbdev.valid = true;
	usbdev.controller = xc;
	usbdev.ctrlr_type = USB_CTRLR_XHCI;
	usbdev.ctrlrID = ctrlrID;
	usbdev.address = 0;
	usbdev.port = port;
	usbdev.max_pkt_size = 8;
	if (!(xhci_set_address(&usbdev)))//,dev_addr)))
		return false;
	usbdev.address = dev_addr;
	usb_dev_desc devdesc = xhci_get_usb_dev_descriptor(&usbdev,sizeof(usb_dev_desc));
	if (!devdesc.length)
		return false;
	usbdev.max_pkt_size = devdesc.max_pkt_size;
	usbdev.driver_function = 0;
	xc->devices[dev_addr] = usbdev;
	return true;
}

bool xhci_usb_get_desc(usb_device *device, void *out, usb_setup_pkt setup_pkt_template, uint16_t size) {	
	
}

void *xhci_create_interval_in(usb_device *device, void *out, uint8_t interval, uint8_t endpoint_addr, uint16_t max_pkt_size, uint16_t size) {
	
}

bool xhci_refresh_interval(void *data) {
	
}

void xhci_destroy_interval(void *data) {
	
}

usb_dev_desc xhci_get_usb_dev_descriptor(usb_device *device, uint16_t size) {	
	usb_dev_desc retval;
	memset(&retval,0,sizeof(usb_dev_desc));
	
	// Generate a setup packet
	usb_setup_pkt setup_pkt_template = {0};
	setup_pkt_template.type = 0x80;
	setup_pkt_template.request = 6;
	setup_pkt_template.value = 0x100;
	setup_pkt_template.index = 0;
	setup_pkt_template.length = size;
	
	if (xhci_usb_get_desc(device,&retval,setup_pkt_template,size))
		dbgprintf("[xHCI] Successfully got USB device descriptor.\n");
	else
		dbgprintf("[xHCI] Failed to get USB Device descriptor\n");
	
	return retval;
}

bool xhci_get_usb_str_desc(usb_device *device, char *out, uint8_t index, uint16_t targetlang) {
	usb_desc_header header;
	usb_setup_pkt sp;
	sp.type = 0x80;
	sp.request = 6;
	sp.value = 0x300;
	sp.index = 0;
	sp.length = 2;
	if (!xhci_usb_get_desc(device,&header,sp,2))
		return false;
	sp.length = header.length;
	char buffer[256];
	memset(buffer,0,256);
	usb_str_desc *strdesc = (usb_str_desc *)buffer;
	if (!xhci_usb_get_desc(device,buffer,sp,header.length))
		return false;
	uint8_t i;
	for (i = 0; i < (strdesc->length-2)/2; i++) {
		if (strdesc->langid[i]==targetlang)
			break;
	}
	if (i==(strdesc->length-2)/2)
		return false;
	
	sp.index = strdesc->langid[i];
	sp.value = 0x300 | index;
	printf("Getting index %d, (0x%#)\n",(uint32_t)index,(uint64_t)sp.value);
	sp.length = 2;
	memset(buffer,0,256);
	if (!xhci_usb_get_desc(device,&header,sp,2))
		return false;
	sp.length = header.length;
	if (!xhci_usb_get_desc(device,buffer,sp,header.length))
		return false;
	uint16_t k = 0;
	for (uint16_t j = 2; j < header.length; j++)
		out[k++]=buffer[j++];
	
	dbgprintf("[xHCI] Successfully got USB string descriptor.\n");
	return true;
}

usb_config_desc xhci_get_config_desc(usb_device *device, uint8_t index) {	
	usb_desc_header header;
	usb_setup_pkt sp;
	memset(&sp,0,sizeof(usb_setup_pkt));
	usb_config_desc config;
	memset(&config,0,sizeof(usb_config_desc));
	sp.type = 0x80;
	sp.request = 6;
	sp.value = 0x200 | index;
	sp.index = 0;
	sp.length = 2;
	if (!xhci_usb_get_desc(device,&header,sp,2))
		return config;
	sp.length = header.length;
	if (!xhci_usb_get_desc(device,&config,sp,header.length))
		return config;
	
	dbgprintf("[xHCI] Successfully got USB config descriptor\n");
	return config;
}

void *xhci_get_next_desc(uint8_t type, void *start, void *end) {
	void *bufferptr = start;
	while (bufferptr<end) {
		usb_desc_header *header = bufferptr;
		if (header->type==type)
			break;
		if (header->length)
			bufferptr += header->length;
		else
			bufferptr = end;
	}
	if (bufferptr>=end) {
		return 0;
	} else
		return bufferptr;
}

usb_interface_desc xhci_get_interface_desc(usb_device *device, uint8_t config_index, uint8_t interface_index) {	
	usb_config_desc config = xhci_get_config_desc(device,config_index);
	usb_interface_desc interface;
	memset(&interface,0,sizeof(usb_interface_desc));
	
	if (config.num_interfaces<interface_index)
		return interface;
	
	usb_setup_pkt sp;
	sp.type = 0x80;
	sp.request = 6;
	sp.value = 0x200 | config_index;
	sp.index = 0;
	sp.length = config.total_len;
	uint16_t num_pages = (config.total_len/4096)+1;
	void *buffer = alloc_page(num_pages);
	void *buffer_end = buffer+(num_pages*4096);
	memset(buffer,0,num_pages*4096);
	if (!xhci_usb_get_desc(device,buffer,sp,config.total_len)) {
		free_page(buffer,num_pages);
		return interface;
	}
	void *bufferptr = buffer+sizeof(usb_config_desc);
	for (uint8_t i = 0; i < interface_index; i++) {
		bufferptr = xhci_get_next_desc(USB_DESC_INTERFACE,bufferptr,buffer_end);
		if (!bufferptr) {
			free_page(buffer,num_pages);
			return interface;
		}
	}
	bufferptr = xhci_get_next_desc(USB_DESC_INTERFACE,bufferptr,buffer_end);
	if (!bufferptr) {
		free_page(buffer,num_pages);
		return interface;
	}
	
	interface = *(usb_interface_desc *)bufferptr;
	
	free_page(buffer,num_pages);
	dbgprintf("[xHCI] Successfully got USB interface descriptor\n");
	return interface;
}

usb_endpoint_desc xhci_get_endpoint_desc(usb_device *device, uint8_t config_index, uint8_t interface_index, uint8_t endpoint_index) {	
	usb_config_desc config = xhci_get_config_desc(device,config_index);
	usb_endpoint_desc endpoint;
	memset(&endpoint,0,sizeof(usb_endpoint_desc));
	
	if (config.num_interfaces<interface_index)
		return endpoint;
	
	usb_setup_pkt sp;
	sp.type = 0x80;
	sp.request = 6;
	sp.value = 0x200 | config_index;
	sp.index = 0;
	sp.length = config.total_len;
	uint16_t num_pages = (config.total_len/4096)+1;
	void *buffer = alloc_page(num_pages);
	void *buffer_end = buffer+(num_pages*4096);
	memset(buffer,0,num_pages*4096);
	if (!xhci_usb_get_desc(device,buffer,sp,config.total_len)) {
		free_page(buffer,num_pages);
		return endpoint;
	}
	dump_memory(buffer,config.total_len);
	void *bufferptr = buffer+sizeof(usb_config_desc);
	for (uint8_t i = 0; i < interface_index; i++) {
		bufferptr = xhci_get_next_desc(USB_DESC_INTERFACE,bufferptr,buffer_end);
		if (!bufferptr) {
			free_page(buffer,num_pages);
			return endpoint;
		}
	}
	bufferptr = xhci_get_next_desc(USB_DESC_INTERFACE,bufferptr,buffer_end);
	if (!bufferptr) {
		free_page(buffer,num_pages);
		return endpoint;
	}
	for (uint8_t i = 0; i < endpoint_index; i++) {
		bufferptr = xhci_get_next_desc(USB_DESC_ENDPOINT,bufferptr,buffer_end);
		if (!bufferptr) {
			free_page(buffer,num_pages);
			return endpoint;
		}
	}
	bufferptr = xhci_get_next_desc(USB_DESC_ENDPOINT,bufferptr,buffer_end);
	if (!bufferptr) {
		free_page(buffer,num_pages);
		return endpoint;
	}
	
	endpoint = *(usb_endpoint_desc *)bufferptr;
	
	dbgprintf("[xHCI] Endpoint: %#\n",(uint64_t)(uint32_t)(bufferptr));
	
	free_page(buffer,num_pages);
	dbgprintf("[xHCI] Successfully got USB endpoint descriptor\n");
	return endpoint;
}