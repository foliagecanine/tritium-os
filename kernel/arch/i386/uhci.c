#include <usb/uhci.h>

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

uhci_controller uhci_controllers[USB_MAX_CTRLRS];
uint8_t num_ctrlrs = 0;

void uhci_global_reset(uint16_t iobase) {
	for (uint8_t i = 0; i < 5; i++) {
		outw(iobase+UHCI_USBCMD,UHCI_USBCMD_GRESET);
		sleep(11);
		outw(iobase+UHCI_USBCMD,0);
	}
	sleep(50);
}

bool uhci_port_reset(uint16_t iobase, uint8_t port) {
	uint16_t portbase = iobase+UHCI_PORTSC1+(port*2);
	if (!(inw(portbase)&UHCI_PORTSC_ALLONE)) {
		return false;
	}
	
	outw(portbase,inw(portbase) & ~(1<<7));
	if (!(inw(portbase)&UHCI_PORTSC_ALLONE)) {
		return false;
	}
	
	outw(portbase,inw(portbase) | (1<<7));
	if (!(inw(portbase)&UHCI_PORTSC_ALLONE)) {
		return false;
	}
	
	outw(portbase,inw(portbase) | UHCI_PORTSC_PEDCHG | UHCI_PORTSC_CSTSCHG);
	if (inw(portbase)&(UHCI_PORTSC_PEDCHG | UHCI_PORTSC_CSTSCHG)) {
		return false;
	}
	
	dbgprintf("Port %d determined valid.\n",(uint32_t)port);
	
	// Assume port exists. Try to reset it.
	outw(portbase,inw(portbase) | UHCI_PORTSC_PRESET); // Issue port reset.
	sleep(50);
	outw(portbase,inw(portbase) & ~UHCI_PORTSC_PRESET);
	
	for (uint8_t i = 0; i < 10; i++) {
		sleep(10);
		
		uint16_t in = inw(portbase);
		
		// If there's nothing attached, it can't clear.
		if (!(in&UHCI_PORTSC_CSTS)) {
			dbgprintf("Port %d reset but nothing attached.\n",(uint32_t)port);
			return true; // Maybe should return false?
		}
		
		// If the enable or connection status changed, clear the bits and wait another cycle.
		if (in & (UHCI_PORTSC_CSTSCHG | UHCI_PORTSC_PEDCHG)) {
			outw(portbase,in & ~(UHCI_PORTSC_CSTSCHG | UHCI_PORTSC_PEDCHG));
			continue;
		}
		
		// Return true if port says it's enabled.
		if (in & UHCI_PORTSC_PE) {
			dbgprintf("Port %d reset and enabled.\n",(uint32_t)port);
			return true;
		}
		
		outw(portbase,in | UHCI_PORTSC_PE);
	}
	
	dbgprintf("Failed to reset port %d.\n",(uint32_t)port);
	return false;
}

void dump_ioregs(uint16_t iobase) {
	dbgprintf("USBCMD: %#\n",(uint64_t)inw(iobase+UHCI_USBCMD));
	dbgprintf("USBSTS: %#\n",(uint64_t)inw(iobase+UHCI_USBSTS));
	dbgprintf("USBINTR: %#\n",(uint64_t)inw(iobase+UHCI_USBINTR));
	dbgprintf("FRNUM: %#\n",(uint64_t)inw(iobase+UHCI_FRNUM));
	dbgprintf("FRBASEADD: %#\n",(uint64_t)inl(iobase+UHCI_FRBASEADD));
	dbgprintf("PORTSC1: %#\n",(uint64_t)inw(iobase+UHCI_PORTSC1));
	dbgprintf("PORTSC2: %#\n",(uint64_t)inw(iobase+UHCI_PORTSC2));
}

void dump_xfr_desc(uhci_usb_xfr_desc td,uint8_t id) {
	dbgprintf("TD%d: %# %# %# %#\n",(uint32_t)id,(uint64_t)td.linkptr,(uint64_t)td.flags0,(uint64_t)td.flags1,(uint64_t)td.bufferptr);
}

void dump_queue(uhci_usb_queue q) {
	dbgprintf("Q: %# %#\n",(uint64_t)q.headlinkptr,(uint64_t)q.elemlinkptr);
}

uint8_t data_table[] = {0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,6,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,7};

uint8_t init_uhci_ctrlr(uint16_t iobase) {
	dbgprintf("IOBase: %#\n",(uint64_t)iobase);
	uhci_global_reset(iobase);
	
	// Check if reset succeeded
	if (!(inw(iobase+UHCI_USBCMD)==0||inw(iobase+UHCI_USBCMD)==UHCI_USBCMD_SWDBG))
		return false;
	
	// Check if the SOF is valid
	outw(iobase+UHCI_USBSTS,0x003F); // Clear all of the status registers.
	if (inb(iobase+UHCI_SOFMOD)!=64)
		return false;
	
	// Reset the host controller
	outw(iobase+UHCI_USBCMD,UHCI_USBCMD_HCRESET);
	sleep(50);
	if (inw(iobase+UHCI_USBCMD)&UHCI_USBCMD_HCRESET)
		return false;
	
	//Setup controller structure
	uint8_t this_ctrlr_id = num_ctrlrs;
	uhci_controller *this_ctrlr = &uhci_controllers[this_ctrlr_id];
	memset(this_ctrlr,0,sizeof(uhci_controller));
	num_ctrlrs++;
	this_ctrlr->iobase = iobase;
	
	//Allocate the USB stack
	this_ctrlr->framelist_vaddr = (uint32_t)alloc_page(2);
	this_ctrlr->framelist_paddr = (uint32_t)get_phys_addr((void *)this_ctrlr->framelist_vaddr);
	dbgprintf("Allocated 2 pages at vaddr:%# paddr:%#\n",(uint64_t)this_ctrlr->framelist_vaddr,(uint64_t)this_ctrlr->framelist_paddr);
	
	//Create a Linked List of queues
	uhci_usb_queue q;
	this_ctrlr->queues_vaddr = (uhci_usb_queue *)(this_ctrlr->framelist_vaddr+4096);
	this_ctrlr->queues_paddr = (uhci_usb_queue *)(this_ctrlr->framelist_paddr+4096);
	q.headlinkptr = UHCI_FRAMEPTR_TERM;
	q.elemlinkptr = UHCI_FRAMEPTR_TERM;
	q.taillinkptr = 0;
	
	for (uint16_t i = 0; i < 8; i++) {
		this_ctrlr->queues_vaddr[i] = q;
		q.headlinkptr = (uint32_t)(this_ctrlr->queues_paddr+4096+(i*sizeof(uhci_usb_queue)));
	}
	
	//Link the transfer descriptors to the queues.
	for (uint16_t i = 0; i < 4096; i+=4) {
		//*(uint32_t *)(this_ctrlr->framelist_vaddr+i) = UHCI_FRAMEPTR_TERM;
		*(uint32_t *)(this_ctrlr->framelist_vaddr+i) = (this_ctrlr->framelist_paddr+4096+(data_table[(i/4)%128]*sizeof(uhci_usb_queue))) | UHCI_FRAMEPTR_QUEUE;
	}
	
	
	//Tell the USB controller to use this memory.
	outl(iobase+UHCI_FRBASEADD, this_ctrlr->framelist_paddr & UHCI_FRBASEADD_BADD);
	outw(iobase+UHCI_FRNUM, 0); //Begin executing at frame 0
	outb(iobase+UHCI_SOFMOD, 0x40); //It should already be this
	outw(iobase+UHCI_USBINTR,UHCI_USBINTR_IOCE); //Turn off all interrupts except IOC
	outw(iobase+UHCI_USBSTS,UHCI_USBSTS_ALLSTS); //Clear all status bits
	outw(iobase+UHCI_USBCMD,UHCI_USBCMD_MAXP | UHCI_USBCMD_CF | UHCI_USBCMD_RS); //Set packet size to 64 bytes, enable the configuration, then set the Run bit.
	//The USB device should now be running through all the framelist addresses. It shouldn't execute anything since we enabled the TERMINATE bit in the queues, which disables execution.
	dbgprintf("Enabled USB stack for controller\n");
	
	uint8_t current_port = 0;
	
	while (uhci_port_reset(iobase,current_port)) {
		dbgprintf("Reset port %d\n",(uint32_t)current_port);
		if (inw(iobase+UHCI_PORTSC1+(current_port*2))&1) {
			dbgprintf("Found device at port %d, geting descriptor...\n",current_port);
			usb_device usbdev;
			memset(&usbdev,0,sizeof(usb_device));
			usbdev.valid = true;
			usbdev.controller = this_ctrlr;
			usbdev.ctrlr_type = USB_CTRLR_UHCI;
			usbdev.address = 0;
			usbdev.port = current_port;
			usbdev.lowspeed = (inw(this_ctrlr->iobase+UHCI_PORTSC1+(current_port*2))&UHCI_PORTSC_LS) ? 1 : 0;
			usbdev.max_pkt_size = 8;
			usbdev.driver_function = 0;
			usb_dev_desc devdesc;
			devdesc = uhci_get_usb_dev_descriptor(&usbdev,8);
			if (devdesc.length) {
				usbdev.max_pkt_size = devdesc.max_pkt_size;
				dbgprintf("Max packet size: %d\n",devdesc.max_pkt_size);
				uhci_port_reset(iobase,current_port);
				uint8_t devaddr = uhci_get_unused_device(this_ctrlr);
				if (uhci_set_address(&usbdev,devaddr)) {
					this_ctrlr->devices[devaddr] = usbdev;
					dbgprintf("Added device at address %d\n",(uint32_t)devaddr);
				} else {
					dbgprintf("Failed to set address\n");
				}
			} else {
				dbgprintf("Failed to get descriptor.\n");
			}
		}
		
		current_port++;
	}
	dbgprintf("Reset %d ports.\n",(uint32_t)current_port);
	this_ctrlr->num_ports = current_port;
	dbgprintf("Finished initializing UHCI controller.\n");
	return num_ctrlrs;
}

uhci_controller *get_uhci_controller(uint8_t id) {
	return &uhci_controllers[id];
}

uint8_t uhci_get_unused_device(uhci_controller *uc) {
	for (uint8_t i = 1; i; i++) {
		if (!(uc->devices[i].valid))
			return i;
	}
	return 0;
}

void uhci_clear_interrupt(uint8_t id) {
	outw(uhci_controllers[id].iobase+UHCI_USBSTS,UHCI_USBSTS_ALLSTS);
}

void uhci_add_queue(uhci_controller *uc, uhci_usb_queue *queue, void *p_queue, uint8_t interval) {
	queue->headlinkptr = uc->queues_vaddr[interval].elemlinkptr;
	queue->taillinkptr = (uint32_t)&uc->queues_vaddr[interval];
	if (!(uc->queues_vaddr[interval].elemlinkptr&UHCI_FRAMEPTR_TERM)) {
		((uhci_usb_queue *)(uc->queues_vaddr[interval].elemlinkptr))->taillinkptr = (uint32_t)queue;
	}
	uc->queues_vaddr[interval].elemlinkptr = (uint32_t)p_queue | UHCI_FRAMEPTR_QUEUE;
}

void uhci_remove_queue(uhci_usb_queue *queue) {
	if (!(queue->headlinkptr&UHCI_FRAMEPTR_TERM))
		((uhci_usb_queue *)(queue->headlinkptr))->taillinkptr = queue->taillinkptr;
	if (!(((uhci_usb_queue *)(queue->taillinkptr))->taillinkptr))
		((uhci_usb_queue *)(queue->taillinkptr))->elemlinkptr = queue->headlinkptr;
	else
		((uhci_usb_queue *)(queue->taillinkptr))->headlinkptr = queue->headlinkptr;
}

bool uhci_generic_setup(usb_device *device, usb_setup_pkt setup_pkt_template) {
	// Allocate a page and define where all our structures will be
	void *data_vaddr = alloc_page(1);
	void *data_paddr = get_phys_addr(data_vaddr);
	uhci_usb_queue *queue = (uhci_usb_queue *)data_vaddr;
	uhci_usb_queue *p_queue = data_paddr;
	uhci_usb_xfr_desc *td = data_vaddr+sizeof(uhci_usb_queue);
	uhci_usb_xfr_desc *p_td = data_paddr+sizeof(uhci_usb_queue);
	uint8_t *setup_pkt = data_vaddr+sizeof(uhci_usb_queue)+(sizeof(uhci_usb_xfr_desc)*2);
	uint8_t *p_setup_pkt = data_paddr+sizeof(uhci_usb_queue)+(sizeof(uhci_usb_xfr_desc)*2);
	uhci_controller *uc = device->controller;
	memcpy(setup_pkt,&setup_pkt_template,8);
	
	queue->headlinkptr = UHCI_FRAMEPTR_TERM;
	queue->elemlinkptr = (uint32_t)p_td;
	
	// Create a TD that points the the setup packet.
	uint8_t i = 0;
	td[i].linkptr = (uint32_t)&p_td[i+1];
	td[i].flags0 = (device->lowspeed*UHCI_XFRDESC_LS) | UHCI_XFRDESC_CERR | UHCI_XFRDESC_STATUS_OFF(0x80);
	td[i].flags1 = UHCI_XFRDESC_MAXLEN_OFF(7) | UHCI_XFRDESC_DEVADDR_OFF(device->address) | UHCI_PID_SETUP;
	td[i].bufferptr = (uint32_t)p_setup_pkt;
	i++;
	
	// Create a status TD
	td[i].linkptr = UHCI_XFRDESC_TERM;
	td[i].flags0 = (device->lowspeed*UHCI_XFRDESC_LS) | UHCI_XFRDESC_CERR | UHCI_XFRDESC_IOC | UHCI_XFRDESC_STATUS_OFF(0x80);
	td[i].flags1 = UHCI_XFRDESC_MAXLEN | UHCI_XFRDESC_DTOGGLE2 | UHCI_XFRDESC_DEVADDR_OFF(device->address) | UHCI_PID_IN;
	td[i].bufferptr = 0;
	i++;
	
	// Clear the interrupt bit and disable IRQ
	outw(uc->iobase+UHCI_USBSTS,UHCI_USBSTS_USBINT);
	outw(uc->iobase+UHCI_USBINTR,0);
	
	// Add our queue to the framelist
	uhci_add_queue(uc,queue,p_queue,0);
	
	// Wait until the TDs are executed (USBINT bit comes on)
	uint16_t timeout = 2000;
	while (!(inw(uc->iobase+UHCI_USBSTS) & UHCI_USBSTS_USBINT) && timeout) {
		timeout--;
		sleep(1);
	}
	
	// Check to make sure we didn't time out
	if (!timeout) {
		dbgprintf("USB timed out.\n");
		uhci_remove_queue(queue);
		outw(uc->iobase+UHCI_USBINTR,UHCI_USBINTR_IOCE);
		free_page(data_vaddr,1);
		return false;
	}
	
	// Clear the interrupt bit and remove our queue from the list, and re-enable IRQs
	outw(uc->iobase+UHCI_USBSTS, UHCI_USBSTS_USBINT);
	uhci_remove_queue(queue);
	outw(uc->iobase+UHCI_USBINTR,UHCI_USBINTR_IOCE);
	
	// Check for any errors in the TDs
	for (uint8_t j = 0; j < i; j++) {
		if (td[j].flags0 & UHCI_XFRDESC_STATUS) {
			dbgprintf("TD Error.\n");
			outw(uc->iobase+UHCI_USBINTR,UHCI_USBINTR_IOCE);
			free_page(data_vaddr,1);
			return false;
		}
	}
	
	// Free the data
	free_page(data_vaddr,1);
	
	dbgprintf("Successfully processed setup packet.\n");
	return true;
}

// Set the address of a usb device.
bool uhci_set_address(usb_device *device, uint8_t dev_address) {
	// Allocate a page and define where all our structures will be
	void *data_vaddr = alloc_page(1);
	void *data_paddr = get_phys_addr(data_vaddr);
	uhci_usb_queue *queue = (uhci_usb_queue *)data_vaddr;
	uhci_usb_queue *p_queue = data_paddr;
	uhci_usb_xfr_desc *td = data_vaddr+sizeof(uhci_usb_queue);
	uhci_usb_xfr_desc *p_td = data_paddr+sizeof(uhci_usb_queue);
	uint8_t *setup_pkt = data_vaddr+sizeof(uhci_usb_queue)+(sizeof(uhci_usb_xfr_desc)*2);
	uint8_t *p_setup_pkt = data_paddr+sizeof(uhci_usb_queue)+(sizeof(uhci_usb_xfr_desc)*2);
	uhci_controller *uc = device->controller;
	
	// Generate a setup packet to change the address
	uint8_t setup_pkt_template[8] = {0,5,0,0,0,0,0,0};
	setup_pkt_template[2] = dev_address;
	memcpy(setup_pkt,setup_pkt_template,8);
	
	queue->headlinkptr = UHCI_FRAMEPTR_TERM;
	queue->elemlinkptr = (uint32_t)p_td;
	
	// Create a TD that points the the setup packet.
	uint8_t i = 0;
	td[i].linkptr = (uint32_t)&p_td[i+1];
	td[i].flags0 = (device->lowspeed*UHCI_XFRDESC_LS) | UHCI_XFRDESC_CERR | UHCI_XFRDESC_STATUS_OFF(0x80);
	td[i].flags1 = UHCI_XFRDESC_MAXLEN_OFF(7) | UHCI_PID_SETUP;
	td[i].bufferptr = (uint32_t)p_setup_pkt;
	i++;
	
	// Create a status TD
	td[i].linkptr = UHCI_XFRDESC_TERM;
	td[i].flags0 = (device->lowspeed*UHCI_XFRDESC_LS) | UHCI_XFRDESC_CERR | UHCI_XFRDESC_IOC | UHCI_XFRDESC_STATUS_OFF(0x80);
	td[i].flags1 = UHCI_XFRDESC_MAXLEN | UHCI_XFRDESC_DTOGGLE2 | UHCI_PID_IN;
	td[i].bufferptr = 0;
	i++;
	
	// Clear the interrupt bit and disable IRQ
	outw(uc->iobase+UHCI_USBSTS,UHCI_USBSTS_USBINT);
	outw(uc->iobase+UHCI_USBINTR,0);
	
	// Add our queue to the framelist
	uhci_add_queue(uc,queue,p_queue,0);
	
	// Wait until the TDs are executed (USBINT bit comes on)
	uint16_t timeout = 2000;
	while (!(inw(uc->iobase+UHCI_USBSTS) & UHCI_USBSTS_USBINT) && timeout) {
		timeout--;
		sleep(1);
	}
	
	// Check to make sure we didn't time out
	if (!timeout) {
		dbgprintf("USB timed out.\n");
		uhci_remove_queue(queue);		
		outw(uc->iobase+UHCI_USBINTR,UHCI_USBINTR_IOCE);
		free_page(data_vaddr,1);
		return false;
	}
	
	// Clear the interrupt bit and remove our queue from the list
	outw(uc->iobase+UHCI_USBSTS, UHCI_USBSTS_USBINT);
	uhci_remove_queue(queue);
	outw(uc->iobase+UHCI_USBINTR,UHCI_USBINTR_IOCE);
	
	// Check for any errors in the TDs
	for (uint8_t j = 0; j < i; j++) {
		if (td[j].flags0 & UHCI_XFRDESC_STATUS) {
			dbgprintf("TD Error.\n");
			outw(uc->iobase+UHCI_USBINTR,UHCI_USBINTR_IOCE);
			free_page(data_vaddr,1);
			return false;
		}
	}
	
	// Free the data
	free_page(data_vaddr,1);
	
	dbgprintf("Successfully set address.\n");
	
	device->address = dev_address;
	return true;
}

bool uhci_assign_address(uint8_t ctrlrID, uint8_t port, uint8_t lowspeed) {
	uhci_controller *uc = get_uhci_controller(ctrlrID);
	uint8_t dev_addr = uhci_get_unused_device(uc);
	usb_device usbdev;
	usbdev.valid = true;
	usbdev.controller = uc;
	usbdev.ctrlr_type = USB_CTRLR_UHCI;
	usbdev.address = 0;
	usbdev.port = port;
	usbdev.lowspeed = lowspeed;
	usbdev.max_pkt_size = 8;
	if (!(uhci_set_address(&usbdev,dev_addr)))
		return false;
	usbdev.address = dev_addr;
	usb_dev_desc devdesc = uhci_get_usb_dev_descriptor(&usbdev,sizeof(usb_dev_desc));
	if (!devdesc.length)
		return false;
	usbdev.max_pkt_size = devdesc.max_pkt_size;
	uc->devices[dev_addr] = usbdev;
	return true;
}

bool uhci_usb_get_desc(usb_device *device, void *out, usb_setup_pkt setup_pkt_template, uint16_t size) {	
	// Calculate the number of TDs needed
	uint16_t num_tds = (size/device->max_pkt_size)+3;
	uint16_t total_size = sizeof(uhci_usb_queue)+(num_tds*sizeof(uhci_usb_xfr_desc))+8+size;
	uint16_t num_pages = (total_size/4096)+1;
	
	// Allocate a page and define where all our structures will be
	void *data_vaddr = alloc_page(num_pages);
	void *data_paddr = get_phys_addr(data_vaddr);
	uhci_usb_queue *queue = (uhci_usb_queue *)data_vaddr;
	uhci_usb_queue *p_queue = data_paddr;
	uhci_usb_xfr_desc *td = data_vaddr+sizeof(uhci_usb_queue);
	uhci_usb_xfr_desc *p_td = data_paddr+sizeof(uhci_usb_queue);
	uint8_t *setup_pkt = data_vaddr+sizeof(uhci_usb_queue)+(sizeof(uhci_usb_xfr_desc)*num_tds);
	uint8_t *p_setup_pkt = data_paddr+sizeof(uhci_usb_queue)+(sizeof(uhci_usb_xfr_desc)*num_tds);
	uint8_t *buffer = data_vaddr+sizeof(uhci_usb_queue)+(sizeof(uhci_usb_xfr_desc)*num_tds)+8;
	uint8_t *p_buffer = data_paddr+sizeof(uhci_usb_queue)+(sizeof(uhci_usb_xfr_desc)*num_tds)+8;
	uhci_controller *uc = device->controller;
	
	// Generate a setup packet
	memcpy(setup_pkt,&setup_pkt_template,8);
	
	// Clear the output buffer
	memset(buffer,0,size);
	
	// Create a queue
	queue->headlinkptr = UHCI_FRAMEPTR_TERM;
	queue->elemlinkptr = (uint32_t)p_td;
		
	// Create the setup TD
	uint8_t i = 0;
	td[i].linkptr = (uint32_t)&p_td[i+1];
	td[i].flags0 = (device->lowspeed*UHCI_XFRDESC_LS) | UHCI_XFRDESC_CERR | UHCI_XFRDESC_STATUS_OFF(0x80);
	td[i].flags1 = UHCI_XFRDESC_MAXLEN_OFF(7) | UHCI_XFRDESC_DEVADDR_OFF(device->address) | UHCI_PID_SETUP;
	td[i].bufferptr = (uint32_t)p_setup_pkt;
	
	// Create the number of TDs required to transfer the data
	uint16_t size_remaining = size;
	for (i = 1; i < num_tds-1 && size_remaining; i++) {
		td[i].linkptr = (uint32_t)&p_td[i+1];
		td[i].flags0 = (device->lowspeed*UHCI_XFRDESC_LS) | UHCI_XFRDESC_CERR | UHCI_XFRDESC_STATUS_OFF(0x80);
		td[i].flags1 = UHCI_XFRDESC_MAXLEN_OFF(size_remaining <= device->max_pkt_size ? size_remaining-1 : device->max_pkt_size-1) | ((i&1)*UHCI_XFRDESC_DTOGGLE2) | UHCI_XFRDESC_DEVADDR_OFF(device->address) | UHCI_PID_IN;
		td[i].bufferptr = (uint32_t)p_buffer+((i-1)*device->max_pkt_size);
		size_remaining -= (size_remaining <= device->max_pkt_size ? size_remaining : device->max_pkt_size);
	}
	
	// Create a status TD
	td[i].linkptr = UHCI_XFRDESC_TERM;
	td[i].flags0 = (device->lowspeed*UHCI_XFRDESC_LS) | UHCI_XFRDESC_CERR | UHCI_XFRDESC_IOC | UHCI_XFRDESC_STATUS_OFF(0x80);
	td[i].flags1 = UHCI_XFRDESC_MAXLEN | UHCI_XFRDESC_DTOGGLE2 | UHCI_XFRDESC_DEVADDR_OFF(device->address) | UHCI_PID_OUT;
	td[i].bufferptr = 0;
	i++;
	
	// Clear the interrupt bit and disable IRQ
	outw(uc->iobase+UHCI_USBSTS,UHCI_USBSTS_USBINT);
	outw(uc->iobase+UHCI_USBINTR,0);
	
	// Add our queue to the list
	uhci_add_queue(uc,queue,p_queue,0);
		
	// Wait until we recieve the IOC
	uint16_t timeout = 2000;
	while ((!(inw(uc->iobase+UHCI_USBSTS) & UHCI_USBSTS_USBINT) || td[0].flags0 & UHCI_XFRDESC_STATUS_ACTIVE) && timeout) {
		timeout--;
		sleep(1);
	}
	
	// Make sure we didn't time out
	if (!timeout) {
		dbgprintf("USB timed out.\n");
		uhci_remove_queue(queue);
		outw(uc->iobase+UHCI_USBINTR,UHCI_USBINTR_IOCE);
		free_page(data_vaddr,num_pages);
		return false;
	}
	// Clear the interupt bit, remove our queue from the list, then re-enable IRQs
	outw(uc->iobase+UHCI_USBSTS, UHCI_USBSTS_USBINT);
	uhci_remove_queue(queue);
	outw(uc->iobase+UHCI_USBINTR,UHCI_USBINTR_IOCE);
	
	// Check to make sure there's no errors with any of the TDs
	for (uint8_t j = 0; j < i; j++) {
		if (td[j].flags0 & UHCI_XFRDESC_STATUS) {
			dbgprintf("TD%d Error: %#\n",(uint32_t)j,(uint64_t)(td[j].flags0));
			outw(uc->iobase+UHCI_USBINTR,UHCI_USBINTR_IOCE);
			free_page(data_vaddr,num_pages);
			return false;
		}
	}
	
	// Copy our data over to the return value
	memcpy(out,buffer,size);
	
	// Free the data page
	free_page(data_vaddr,num_pages);
	
	dbgprintf("Successfully got USB descriptor.\n");
	return true;
}

void *uhci_create_interval_in(usb_device *device, void *out, uint8_t interval, uint8_t endpoint_addr, uint16_t max_pkt_size, uint16_t size) {
	// Allocate a page and define where all our structures will be
	void *data_vaddr = alloc_page(1);
	void *data_paddr = get_phys_addr(data_vaddr);
	uhci_usb_queue *queue = (uhci_usb_queue *)data_vaddr;
	uhci_usb_queue *p_queue = data_paddr;
	uint16_t *num_tds = data_vaddr+sizeof(uhci_usb_queue);
	uhci_usb_xfr_desc *td = data_vaddr+sizeof(uhci_usb_queue)+16;
	uhci_usb_xfr_desc *p_td = data_paddr+sizeof(uhci_usb_queue)+16;
	uint8_t *buffer = out;
	uint8_t *p_buffer = get_phys_addr(out);
	uhci_controller *uc = device->controller;
	
	// Clear the output buffer and the allocation page
	memset(buffer,0,size);
	memset(data_vaddr,0,4096);
	
	// Calculate the number of TDs needed
	*num_tds = (size/max_pkt_size);
	if (size%max_pkt_size)
		*num_tds++;
	
	// Create a queue
	queue->headlinkptr = UHCI_FRAMEPTR_TERM;
	queue->elemlinkptr = (uint32_t)p_td;
	queue->taillinkptr = 0;
	
	// No setup TD since this is an interrupt transfer
	
	// Create the number of TDs required to transfer the data
	uint8_t i;
	uint16_t size_remaining = size;
	for (i = 0; i < *num_tds && size_remaining; i++) {
		td[i].linkptr = (uint32_t)&p_td[i+1];
		td[i].flags0 = (device->lowspeed*UHCI_XFRDESC_LS) | UHCI_XFRDESC_CERR | UHCI_XFRDESC_STATUS_ACTIVE;
		td[i].flags1 = UHCI_XFRDESC_MAXLEN_OFF(size_remaining <= device->max_pkt_size ? size_remaining-1 : device->max_pkt_size-1) | ((i&1)*UHCI_XFRDESC_DTOGGLE2) | UHCI_XFRDESC_DEVADDR_OFF(device->address) | UHCI_XFRDESC_ENDPT_OFF(endpoint_addr) | UHCI_PID_IN;
		td[i].bufferptr = (uint32_t)p_buffer+(i*device->max_pkt_size);
		size_remaining -= (size_remaining <= device->max_pkt_size ? size_remaining : device->max_pkt_size);
	}
	td[i-1].linkptr = UHCI_XFRDESC_TERM;
	td[i-1].flags0 |= UHCI_XFRDESC_IOC;
	// No status TD
	
	// Add our queue to the list
	uhci_add_queue(uc,queue,p_queue,interval);
	
	// Make sure interrupts are enabled
	outw(((uhci_controller *)device->controller)->iobase+UHCI_USBINTR,UHCI_USBINTR_IOCE);
	
	return data_vaddr;
}

bool uhci_refresh_interval(void *data) {
	uhci_usb_queue *queue = data;
	uint16_t *num_tds = data+sizeof(uhci_usb_queue);
	uhci_usb_xfr_desc *td = data+sizeof(uhci_usb_queue)+16;
	bool retval = true;
	for (uint16_t i = 0; i < *num_tds; i++) {
		if (td[i].flags0 & UHCI_XFRDESC_STATUS_ACTIVE)
			retval = false;
		else
			td[i].flags0 |= UHCI_XFRDESC_STATUS_ACTIVE;
	}
	queue->elemlinkptr = (uint32_t)get_phys_addr(td);
	return retval;
}

void uhci_destroy_interval(void *data) {
	uhci_remove_queue(data);
	free_page(data,1);
}

usb_dev_desc uhci_get_usb_dev_descriptor(usb_device *device, uint16_t size) {	
	usb_dev_desc retval;
	memset(&retval,0,sizeof(usb_dev_desc));
	
	// Generate a setup packet
	usb_setup_pkt setup_pkt_template = {0};
	setup_pkt_template.type = 0x80;
	setup_pkt_template.request = 6;
	setup_pkt_template.value = 0x100;
	setup_pkt_template.index = 0;
	setup_pkt_template.length = size;
	
	uhci_usb_get_desc(device,&retval,setup_pkt_template,size);
	
	dbgprintf("Successfully got USB device descriptor.\n");
	
	return retval;
}

bool uhci_get_usb_str_desc(usb_device *device, char *out, uint8_t index, uint16_t targetlang) {
	usb_desc_header header;
	usb_setup_pkt sp;
	sp.type = 0x80;
	sp.request = 6;
	sp.value = 0x300;
	sp.index = 0;
	sp.length = 2;
	if (!uhci_usb_get_desc(device,&header,sp,2))
		return false;
	sp.length = header.length;
	char buffer[256];
	memset(buffer,0,256);
	usb_str_desc *strdesc = (usb_str_desc *)buffer;
	if (!uhci_usb_get_desc(device,buffer,sp,header.length))
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
	if (!uhci_usb_get_desc(device,&header,sp,2))
		return false;
	sp.length = header.length;
	if (!uhci_usb_get_desc(device,buffer,sp,header.length))
		return false;
	uint16_t k = 0;
	for (uint16_t j = 2; j < header.length; j++)
		out[k++]=buffer[j++];
	
	dbgprintf("Successfully got USB string descriptor.\n");
	return true;
}

usb_config_desc uhci_get_config_desc(usb_device *device, uint8_t index) {	
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
	if (!uhci_usb_get_desc(device,&header,sp,2))
		return config;
	sp.length = header.length;
	if (!uhci_usb_get_desc(device,&config,sp,header.length))
		return config;
	
	dbgprintf("Successfully got USB config descriptor\n");
	return config;
}

void *uhci_get_next_desc(uint8_t type, void *start, void *end) {
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

usb_interface_desc uhci_get_interface_desc(usb_device *device, uint8_t config_index, uint8_t interface_index) {	
	usb_config_desc config = uhci_get_config_desc(device,config_index);
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
	if (!uhci_usb_get_desc(device,buffer,sp,config.total_len)) {
		free_page(buffer,num_pages);
		return interface;
	}
	void *bufferptr = buffer+sizeof(usb_config_desc);
	for (uint8_t i = 0; i < interface_index; i++) {
		bufferptr = uhci_get_next_desc(USB_DESC_INTERFACE,bufferptr,buffer_end);
		if (!bufferptr) {
			free_page(buffer,num_pages);
			return interface;
		}
	}
	bufferptr = uhci_get_next_desc(USB_DESC_INTERFACE,bufferptr,buffer_end);
	if (!bufferptr) {
		free_page(buffer,num_pages);
		return interface;
	}
	
	interface = *(usb_interface_desc *)bufferptr;
	
	free_page(buffer,num_pages);
	dbgprintf("Successfully got USB interface descriptor\n");
	return interface;
}

void dump_memory(uint8_t *mem, size_t size) {
	for (size_t i = 0; i < size; i++) {
		if (i&&!(i%16))
			dbgprintf("\n");
		if (mem[i]<0x10)
			dbgprintf("0");
		dbgprintf("%# ",(uint64_t)mem[i]);
	}
}

usb_endpoint_desc uhci_get_endpoint_desc(usb_device *device, uint8_t config_index, uint8_t interface_index, uint8_t endpoint_index) {	
	usb_config_desc config = uhci_get_config_desc(device,config_index);
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
	if (!uhci_usb_get_desc(device,buffer,sp,config.total_len)) {
		free_page(buffer,num_pages);
		return endpoint;
	}
	dump_memory(buffer,config.total_len);
	void *bufferptr = buffer+sizeof(usb_config_desc);
	for (uint8_t i = 0; i < interface_index; i++) {
		bufferptr = uhci_get_next_desc(USB_DESC_INTERFACE,bufferptr,buffer_end);
		if (!bufferptr) {
			free_page(buffer,num_pages);
			return endpoint;
		}
	}
	bufferptr = uhci_get_next_desc(USB_DESC_INTERFACE,bufferptr,buffer_end);
	if (!bufferptr) {
		free_page(buffer,num_pages);
		return endpoint;
	}
	for (uint8_t i = 0; i < endpoint_index; i++) {
		bufferptr = uhci_get_next_desc(USB_DESC_ENDPOINT,bufferptr,buffer_end);
		if (!bufferptr) {
			free_page(buffer,num_pages);
			return endpoint;
		}
	}
	bufferptr = uhci_get_next_desc(USB_DESC_ENDPOINT,bufferptr,buffer_end);
	if (!bufferptr) {
		free_page(buffer,num_pages);
		return endpoint;
	}
	
	endpoint = *(usb_endpoint_desc *)bufferptr;
	
	dbgprintf("Endpoint: %#\n",(uint64_t)(uint32_t)(bufferptr));
	
	free_page(buffer,num_pages);
	dbgprintf("Successfully got USB endpoint descriptor\n");
	return endpoint;
}