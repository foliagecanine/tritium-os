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

uhci_controller uhci_controllers[8]; //Allow up to 8 controllers.
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
	outw(iobase+UHCI_USBINTR,0); //Turn off all interrupts
	outw(iobase+UHCI_USBSTS,UHCI_USBSTS_ALLSTS); //Clear all status bits
	outw(iobase+UHCI_USBCMD,UHCI_USBCMD_MAXP | UHCI_USBCMD_CF | UHCI_USBCMD_RS); //Set packet size to 64 bytes, enable the configuration, then set the Run bit.
	//The USB device should now be running through all the framelist addresses. It shouldn't execute anything since we enabled the TERMINATE bit in the queues, which disables execution.
	dbgprintf("Enabled USB stack for controller\n");
	
	uint8_t current_port = 0;
	uint8_t devaddr = 1;
	
	while (uhci_port_reset(iobase,current_port)) {
		dbgprintf("Reset port %d\n",(uint32_t)current_port);
		if (inw(iobase+UHCI_PORTSC1+(current_port*2))&1) {
			dbgprintf("Found device at port %d, geting descriptor...\n",current_port);
			usb_dev_desc usbdev;
			usbdev = uhci_get_usb_dev_descriptor(*this_ctrlr,current_port,0,8,8);
			if (usbdev.length) {
				uhci_port_reset(iobase,current_port);
				if (uhci_set_address(*this_ctrlr,current_port,devaddr)) {
					usbdev = uhci_get_usb_dev_descriptor(*this_ctrlr,current_port,devaddr,usbdev.max_pkt_size,usbdev.length);
					if (usbdev.length) {
						printf("----USB-DEVICE----\n");
						printf("Length: %d\n",(uint32_t)usbdev.length);
						printf("Descriptor: %#\n",(uint64_t)usbdev.desc_type);
						printf("USB Version (BCD): %#\n",(uint64_t)usbdev.usbver_bcd);
						printf("Device Class: %#\n",(uint64_t)usbdev.dev_class);
						printf("Device Subclass: %#\n",(uint64_t)usbdev.dev_subclass);
						printf("Device Protocol: %#\n",(uint64_t)usbdev.dev_protocol);
						printf("Max Packet Size: %d\n",(uint32_t)usbdev.max_pkt_size);
						printf("VendorID: %#\n",(uint64_t)usbdev.vendorID);
						printf("ProductID: %#\n",(uint64_t)usbdev.productID);
						printf("Manufacturer: ");
						char buffer[256];
						if (!uhci_get_string_desc(buffer,usbdev.manuf_index,0x409,*this_ctrlr,current_port,devaddr,usbdev.max_pkt_size))
							printf("Error\n");
						printf("ProductID: ");
						if (!uhci_get_string_desc(buffer,usbdev.product_index,0x409,*this_ctrlr,current_port,devaddr,usbdev.max_pkt_size))
							printf("Error\n");
						printf("Serial Number: ");
						if (!uhci_get_string_desc(buffer,usbdev.sernum_index,0x409,*this_ctrlr,current_port,devaddr,usbdev.max_pkt_size))
							printf("Error\n");
					} else {
						dbgprintf("Failed to get descriptor.\n");
					}
					devaddr++;
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

uhci_controller get_uhci_controller(uint8_t id) {
	return uhci_controllers[id];
}

// Set the address of a usb device.
bool uhci_set_address(uhci_controller uc, uint8_t port, uint8_t dev_address) {
	dbgprintf("usa(uc,%d,%d)\n",(uint32_t)port,(uint32_t)dev_address);
	
	// Allocate a page and define where all our structures will be
	void *data_vaddr = alloc_page(1);
	void *data_paddr = get_phys_addr(data_vaddr);
	uhci_usb_queue *queue = (uhci_usb_queue *)data_vaddr;
	uhci_usb_queue *p_queue = data_paddr;
	uhci_usb_xfr_desc *td = data_vaddr+sizeof(uhci_usb_queue);
	uhci_usb_xfr_desc *p_td = data_paddr+sizeof(uhci_usb_queue);
	uint8_t *setup_pkt = data_vaddr+sizeof(uhci_usb_queue)+(sizeof(uhci_usb_xfr_desc)*2);
	uint8_t *p_setup_pkt = data_paddr+sizeof(uhci_usb_queue)+(sizeof(uhci_usb_xfr_desc)*2);
	
	// Generate a setup packet to change the address
	uint8_t setup_pkt_template[8] = {0,5,0,0,0,0,0,0};
	setup_pkt_template[2] = dev_address;
	memcpy(setup_pkt,setup_pkt_template,8);
	
	queue->headlinkptr = UHCI_FRAMEPTR_TERM;
	queue->elemlinkptr = (uint32_t)p_td;
	
	// Check whether the device is low-speed
	bool lowspeed = 0;
	if (inw(uc.iobase+UHCI_PORTSC1+(port*2)) & UHCI_PORTSC_LS)
		lowspeed = 1;
	
	// Create a TD that points the the setup packet.
	uint8_t i = 0;
	td[i].linkptr = (uint32_t)&p_td[i+1];
	td[i].flags0 = (lowspeed*UHCI_XFRDESC_LS) | UHCI_XFRDESC_CERR | UHCI_XFRDESC_STATUS_OFF(0x80);
	td[i].flags1 = UHCI_XFRDESC_MAXLEN_OFF(7) | UHCI_PID_SETUP;
	td[i].bufferptr = (uint32_t)p_setup_pkt;
	i++;
	
	// Create a status TD
	td[i].linkptr = UHCI_XFRDESC_TERM;
	td[i].flags0 = (lowspeed*UHCI_XFRDESC_LS) | UHCI_XFRDESC_CERR | UHCI_XFRDESC_IOC | UHCI_XFRDESC_STATUS_OFF(0x80);
	td[i].flags1 = UHCI_XFRDESC_MAXLEN | UHCI_XFRDESC_DTOGGLE2 | UHCI_PID_IN;
	td[i].bufferptr = 0;
	i++;
	
	dump_queue(*queue);
	dump_xfr_desc(td[0],0);
	dump_xfr_desc(td[1],1);
	
	// Clear the interrupt bit
	outw(uc.iobase+UHCI_USBSTS,UHCI_USBSTS_USBINT);
	
	// Add our queue to the framelist
	queue->headlinkptr = uc.queues_vaddr[0].elemlinkptr;
	uc.queues_vaddr[0].elemlinkptr = (uint32_t)p_queue | UHCI_FRAMEPTR_QUEUE;
	
	// Wait until the TDs are executed (USBINT bit comes on)
	uint16_t timeout = 10000;
	while (!(inw(uc.iobase+UHCI_USBSTS) & UHCI_USBSTS_USBINT) && timeout) {
		timeout--;
		sleep(1);
	}
	
	// Check to make sure we didn't time out
	if (!timeout) {
		printf("USB timed out.\n");
		uc.queues_vaddr[0].elemlinkptr = queue->headlinkptr;
		return false;
	}
	
	// Clear the interrupt bit and remove our queue from the list
	outw(uc.iobase+UHCI_USBSTS, UHCI_USBSTS_USBINT);
	uc.queues_vaddr[0].elemlinkptr = queue->headlinkptr;
	
	// Check for any errors in the TDs
	for (uint8_t j = 0; j < i; j++) {
		if (td[j].flags0 & UHCI_XFRDESC_STATUS) {
			dbgprintf("TD Error.\n");
			return false;
		}
	}
	
	// Free the data
	free_page(data_vaddr,1);
	
	dbgprintf("Successfully set address.\n");
	
	return true;
}

usb_dev_desc uhci_get_usb_dev_descriptor(uhci_controller uc, uint8_t port, uint8_t dev_address, uint16_t pkt_size, uint16_t size) {
	dbgprintf("ugudd(uc,%d,%d,%d,%d)\n",(uint32_t)port,(uint32_t)dev_address,(uint32_t)pkt_size,(uint32_t)size);
	
	usb_dev_desc retval = {0};
	
	// Allocate a page and define where all our structures will be
	void *data_vaddr = alloc_page(1);
	void *data_paddr = get_phys_addr(data_vaddr);
	uhci_usb_queue *queue = (uhci_usb_queue *)data_vaddr;
	uhci_usb_queue *p_queue = data_paddr;
	uhci_usb_xfr_desc *td = data_vaddr+sizeof(uhci_usb_queue);
	uhci_usb_xfr_desc *p_td = data_paddr+sizeof(uhci_usb_queue);
	uint8_t *setup_pkt = data_vaddr+sizeof(uhci_usb_queue)+(sizeof(uhci_usb_xfr_desc)*10);
	uint8_t *p_setup_pkt = data_paddr+sizeof(uhci_usb_queue)+(sizeof(uhci_usb_xfr_desc)*10);
	uint8_t *buffer = data_vaddr+sizeof(uhci_usb_queue)+(sizeof(uhci_usb_xfr_desc)*10)+8;
	uint8_t *p_buffer = data_paddr+sizeof(uhci_usb_queue)+(sizeof(uhci_usb_xfr_desc)*10)+8;
	
	// Generate a setup packet
	usb_setup_pkt setup_pkt_template = {0};
	setup_pkt_template.type = 0x80;
	setup_pkt_template.request = 6;
	setup_pkt_template.value = 0x100;
	setup_pkt_template.index = 0;
	setup_pkt_template.length = size;
	memcpy(setup_pkt,&setup_pkt_template,8);
	
	// Clear the output buffer
	memset(buffer,0,120);
	
	// Create a queue
	queue->headlinkptr = UHCI_FRAMEPTR_TERM;
	queue->elemlinkptr = (uint32_t)p_td;
	
	// Check whether the device is low-speed
	bool lowspeed = 0;
	if (inw(uc.iobase+UHCI_PORTSC1+(port*2)) & UHCI_PORTSC_LS)
		lowspeed = 1;
	
	// Create the setup TD
	uint8_t i = 0;
	td[i].linkptr = (uint32_t)&p_td[i+1];
	td[i].flags0 = (lowspeed*UHCI_XFRDESC_LS) | UHCI_XFRDESC_CERR | UHCI_XFRDESC_STATUS_OFF(0x80);
	td[i].flags1 = UHCI_XFRDESC_MAXLEN_OFF(7) | UHCI_XFRDESC_DEVADDR_OFF(dev_address) | UHCI_PID_SETUP;
	td[i].bufferptr = (uint32_t)p_setup_pkt;
	
	// Create the number of TDs required to transfer the data
	uint16_t size_remaining = size;
	for (i = 1; i < 9 && size_remaining; i++) {
		td[i].linkptr = (uint32_t)&p_td[i+1];
		td[i].flags0 = (lowspeed*UHCI_XFRDESC_LS) | UHCI_XFRDESC_CERR | UHCI_XFRDESC_STATUS_OFF(0x80);
		td[i].flags1 = UHCI_XFRDESC_MAXLEN_OFF(size_remaining <= pkt_size ? size_remaining-1 : pkt_size-1) | ((i&1)*UHCI_XFRDESC_DTOGGLE2) | UHCI_XFRDESC_DEVADDR_OFF(dev_address) | UHCI_PID_IN;
		td[i].bufferptr = (uint32_t)p_buffer+((i-1)*8);
		size_remaining -= (size_remaining <= pkt_size ? size_remaining : pkt_size);
	}
	
	// Create a status TD
	td[i].linkptr = UHCI_XFRDESC_TERM;
	td[i].flags0 = (lowspeed*UHCI_XFRDESC_LS) | UHCI_XFRDESC_CERR | UHCI_XFRDESC_IOC | UHCI_XFRDESC_STATUS_OFF(0x80);
	td[i].flags1 = UHCI_XFRDESC_MAXLEN | UHCI_XFRDESC_DTOGGLE2 | UHCI_XFRDESC_DEVADDR_OFF(dev_address) | UHCI_PID_OUT;
	td[i].bufferptr = 0;
	i++;
	
	dump_queue(*queue);
	for (uint8_t k = 0; k < i; k++) {
		dump_xfr_desc(td[k],k);
	}
	
	// Clear the interrupt bit
	outw(uc.iobase+UHCI_USBSTS,UHCI_USBSTS_USBINT);
	
	// Add our queue to the list
	queue->headlinkptr = uc.queues_vaddr[0].elemlinkptr;
	uc.queues_vaddr[0].elemlinkptr = (uint32_t)p_queue | UHCI_FRAMEPTR_QUEUE;
	
	// Wait until we recieve the IOC
	uint16_t timeout = 10000;
	while (!(inw(uc.iobase+UHCI_USBSTS) & UHCI_USBSTS_USBINT) && timeout) {
		timeout--;
		sleep(1);
	}
	
	// Make sure we didn't time out
	if (!timeout) {
		printf("USB timed out.\n");
		uc.queues_vaddr[0].elemlinkptr = queue->headlinkptr;
		return retval;
	}
	// Clear the interupt bit and remove our queue from the list
	outw(uc.iobase+UHCI_USBSTS, UHCI_USBSTS_USBINT);
	uc.queues_vaddr[0].elemlinkptr = queue->headlinkptr;
	
	// Check to make sure there's no errors with any of the TDs
	for (uint8_t j = 0; j < i; j++) {
		if (td[j].flags0 & UHCI_XFRDESC_STATUS) {
			dbgprintf("TD%d Error: %#\n",(uint32_t)j,(uint64_t)(td[j].flags0));
			return retval;
		}
	}
	
	// Copy our data over to the return value
	memcpy(&retval,buffer,sizeof(usb_dev_desc));
	
	// Free the data page
	free_page(data_vaddr,1);
	
	dbgprintf("Successfully got USB device descriptor.\n");
	
	return retval;
}

bool uhci_usb_get_desc(void *out, usb_setup_pkt setup_pkt_template, uhci_controller uc, uint8_t port, uint8_t dev_address, uint16_t pkt_size, uint16_t size) {
	dbgprintf("uugd(%#,%#,%d,%d,%d,%d)\n",(uint64_t)(uint32_t)out,(uint64_t)(uint32_t)&setup_pkt_template,(uint32_t)port,(uint32_t)dev_address,(uint32_t)pkt_size,(uint32_t)size);
	
	// Calculate the number of TDs needed
	uint16_t num_tds = (size/pkt_size)+3;
	
	// Allocate a page and define where all our structures will be
	void *data_vaddr = alloc_page(1);
	void *data_paddr = get_phys_addr(data_vaddr);
	uhci_usb_queue *queue = (uhci_usb_queue *)data_vaddr;
	uhci_usb_queue *p_queue = data_paddr;
	uhci_usb_xfr_desc *td = data_vaddr+sizeof(uhci_usb_queue);
	uhci_usb_xfr_desc *p_td = data_paddr+sizeof(uhci_usb_queue);
	uint8_t *setup_pkt = data_vaddr+sizeof(uhci_usb_queue)+(sizeof(uhci_usb_xfr_desc)*num_tds);
	uint8_t *p_setup_pkt = data_paddr+sizeof(uhci_usb_queue)+(sizeof(uhci_usb_xfr_desc)*num_tds);
	uint8_t *buffer = data_vaddr+sizeof(uhci_usb_queue)+(sizeof(uhci_usb_xfr_desc)*num_tds)+8;
	uint8_t *p_buffer = data_paddr+sizeof(uhci_usb_queue)+(sizeof(uhci_usb_xfr_desc)*num_tds)+8;
	
	// Generate a setup packet
	memcpy(setup_pkt,&setup_pkt_template,8);
	
	// Clear the output buffer
	memset(buffer,0,size);
	
	// Create a queue
	queue->headlinkptr = UHCI_FRAMEPTR_TERM;
	queue->elemlinkptr = (uint32_t)p_td;
	
	// Check whether the device is low-speed
	bool lowspeed = 0;
	if (inw(uc.iobase+UHCI_PORTSC1+(port*2)) & UHCI_PORTSC_LS)
		lowspeed = 1;
	
	// Create the setup TD
	uint8_t i = 0;
	td[i].linkptr = (uint32_t)&p_td[i+1];
	td[i].flags0 = (lowspeed*UHCI_XFRDESC_LS) | UHCI_XFRDESC_CERR | UHCI_XFRDESC_STATUS_OFF(0x80);
	td[i].flags1 = UHCI_XFRDESC_MAXLEN_OFF(7) | UHCI_XFRDESC_DEVADDR_OFF(dev_address) | UHCI_PID_SETUP;
	td[i].bufferptr = (uint32_t)p_setup_pkt;
	
	// Create the number of TDs required to transfer the data
	uint16_t size_remaining = size;
	for (i = 1; i < num_tds-1 && size_remaining; i++) {
		td[i].linkptr = (uint32_t)&p_td[i+1];
		td[i].flags0 = (lowspeed*UHCI_XFRDESC_LS) | UHCI_XFRDESC_CERR | UHCI_XFRDESC_STATUS_OFF(0x80);
		td[i].flags1 = UHCI_XFRDESC_MAXLEN_OFF(size_remaining <= pkt_size ? size_remaining-1 : pkt_size-1) | ((i&1)*UHCI_XFRDESC_DTOGGLE2) | UHCI_XFRDESC_DEVADDR_OFF(dev_address) | UHCI_PID_IN;
		td[i].bufferptr = (uint32_t)p_buffer+((i-1)*8);
		size_remaining -= (size_remaining <= pkt_size ? size_remaining : pkt_size);
	}
	
	// Create a status TD
	td[i].linkptr = UHCI_XFRDESC_TERM;
	td[i].flags0 = (lowspeed*UHCI_XFRDESC_LS) | UHCI_XFRDESC_CERR | UHCI_XFRDESC_IOC | UHCI_XFRDESC_STATUS_OFF(0x80);
	td[i].flags1 = UHCI_XFRDESC_MAXLEN | UHCI_XFRDESC_DTOGGLE2 | UHCI_XFRDESC_DEVADDR_OFF(dev_address) | UHCI_PID_OUT;
	td[i].bufferptr = 0;
	i++;
	
	dump_queue(*queue);
	for (uint8_t k = 0; k < i; k++) {
		dump_xfr_desc(td[k],k);
	}
	
	// Clear the interrupt bit
	outw(uc.iobase+UHCI_USBSTS,UHCI_USBSTS_USBINT);
	
	// Add our queue to the list
	queue->headlinkptr = uc.queues_vaddr[0].elemlinkptr;
	uc.queues_vaddr[0].elemlinkptr = (uint32_t)p_queue | UHCI_FRAMEPTR_QUEUE;
	
	// Wait until we recieve the IOC
	uint16_t timeout = 10000;
	while (!(inw(uc.iobase+UHCI_USBSTS) & UHCI_USBSTS_USBINT) && timeout) {
		timeout--;
		sleep(1);
	}
	
	// Make sure we didn't time out
	if (!timeout) {
		printf("USB timed out.\n");
		uc.queues_vaddr[0].elemlinkptr = queue->headlinkptr;
		return false;
	}
	// Clear the interupt bit and remove our queue from the list
	outw(uc.iobase+UHCI_USBSTS, UHCI_USBSTS_USBINT);
	uc.queues_vaddr[0].elemlinkptr = queue->headlinkptr;
	
	// Check to make sure there's no errors with any of the TDs
	for (uint8_t j = 0; j < i; j++) {
		if (td[j].flags0 & UHCI_XFRDESC_STATUS) {
			dbgprintf("TD%d Error: %#\n",(uint32_t)j,(uint64_t)(td[j].flags0));
			return false;
		}
	}
	
	// Copy our data over to the return value
	memcpy(out,buffer,size);
	
	// Free the data page
	free_page(data_vaddr,1);
	
	dbgprintf("Successfully got USB descriptor.\n");
	return true;
}

bool uhci_get_string_desc(char *out, uint8_t index, uint16_t targetlang, uhci_controller uc, uint8_t port, uint8_t dev_address, uint16_t pkt_size) {
	usb_desc_header header;
	usb_setup_pkt sp;
	sp.type = 0x80;
	sp.request = 6;
	sp.value = 0x300;
	sp.index = 0;
	sp.length = 2;
	if (!uhci_usb_get_desc(&header,sp,uc,port,dev_address,pkt_size,2))
		return false;
	sp.length = header.length;
	char buffer[256];
	memset(buffer,0,256);
	usb_str_desc *strdesc = (usb_str_desc *)buffer;
	if (!uhci_usb_get_desc(buffer,sp,uc,port,dev_address,pkt_size,header.length))
		return false;
	dbgprintf("Length: %d\n",(uint32_t)(strdesc->length));
	uint8_t i;
	for (i = 0; i < (strdesc->length-2)/2; i++) {
		dbgprintf("Got LANGID %#\n",(uint64_t)strdesc->langid[i]);
		if (strdesc->langid[i]==targetlang)
			break;
	}
	if (i==(strdesc->length-2)/2)
		return false;
	
	dbgprintf("Achieved target lang. %# == %#\n",(uint64_t)strdesc->langid[i],(uint64_t)targetlang);
	
	sp.index = strdesc->langid[i];
	sp.value = 0x300 | index;
	sp.length = 2;
	memset(buffer,0,256);
	if (!uhci_usb_get_desc(&header,sp,uc,port,dev_address,pkt_size,2))
		return false;
	sp.length = header.length;
	if (!uhci_usb_get_desc(buffer,sp,uc,port,dev_address,pkt_size,header.length))
		return false;
	for (uint16_t j = 2; j < header.length; j++) {
		putchar(buffer[j]);
		j++;
	}
	printf("\n");
	
	dbgprintf("Successfully got USB string descriptor.\n");
	return true;
}