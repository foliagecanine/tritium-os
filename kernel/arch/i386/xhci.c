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

// Force a 32 bit read
inline volatile uint32_t _rd32(volatile void *mem) {
	return *(volatile uint32_t *)mem;
}

// Force a 16 bit read
inline volatile uint16_t _rd16(volatile void *mem) {
	return (uint16_t)((*(volatile uint32_t *)(void *)((uint32_t)mem&~3))>>(((uint32_t)mem&3)*8));;
}

// Force an 8 bit read
inline volatile uint8_t _rd8(volatile void *mem) {
	return (uint8_t)((*(volatile uint32_t *)(void *)((uint32_t)mem&~3))>>(((uint32_t)mem&3)*8));;
}

// Force a 32 bit write
inline void _wr32(void *mem, uint32_t b) {
	__asm__ __volatile__("movl %%eax, %0":"=m"(mem):"a"(b):"memory");
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
	
	uint32_t paramoff = (_rd32(xc->baseaddr+XHCI_HCCAP_HCCPARAM1)>>16)*4;
	_wr32(xc->baseaddr+paramoff,_rd32(xc->baseaddr+paramoff) | XHCI_LEGACY_OWNED);
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
	uint8_t portID;
	uint8_t count = 0;
	uint8_t flags;
	 
}

bool xhci_port_reset() {
	
}

uint8_t init_xhci_ctrlr(uint32_t baseaddr) {
	dbgprintf("[xHCI] Base Address: %#\n",(uint64_t)baseaddr);
	for (uint8_t i = 0; i < 16; i++) {
		identity_map((void *)baseaddr+(i*4096));
	}
	
	xhci_controller *this_ctrlr = get_xhci_controller(xhci_num_ctrlrs++);
	this_ctrlr->baseaddr = (void *)baseaddr;
	this_ctrlr->hcops = (void *)baseaddr+_rd8(this_ctrlr->baseaddr+XHCI_HCCAP_CAPLEN);
	dbgprintf("[xHCI] Ops Address: %#\n",(uint64_t)(uint32_t)this_ctrlr->hcops);
	
	uint16_t hcver = _rd16(this_ctrlr->baseaddr+XHCI_HCCAP_HCIVER);
	if (hcver<0x95)
		return false;
	else
		dbgprintf("[xHCI] Controller version: %#\n",(uint64_t)hcver);
	
	if (!xhci_global_reset(this_ctrlr))
		return false;
	
	xhci_pair_ports(this_ctrlr);
	
	this_ctrlr->num_ports = _rd32(this_ctrlr->baseaddr+XHCI_HCCAP_HCSPARAM1)>>24;
	dbgprintf("[xHCI] Detected %d ports\n",this_ctrlr->num_ports);
	
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

bool xhci_generic_setup(usb_device *device, usb_setup_pkt setup_pkt_template) {
	
}

// Set the address of a usb device.
bool xhci_set_address(usb_device *device, uint8_t dev_address) {
	
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
	if (!(xhci_set_address(&usbdev,dev_addr)))
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