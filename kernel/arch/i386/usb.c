#include <usb/usb.h>

char name[5];

uint8_t ctrlrcounts[4] = {0,0,0,0};

void check_pci(pci_t pci, uint16_t i, uint8_t j, uint8_t k) {
	if (pci.vendorID!=0xFFFF&&pci.classCode==0xC&&pci.subclass==3) {
		if (pci.progIF==0x00)
			strcpy(name,"UHCI");
		else if (pci.progIF==0x10)
			strcpy(name,"OHCI");
		else if (pci.progIF==0x20)
			strcpy(name,"EHCI");
		else if (pci.progIF==0x30)
			strcpy(name,"xHCI");
		else
			strcpy(name,"ERR!");
		if (k==0)
			printf("Detected USB %s Controller on port %#:%#\n", name, (uint64_t)i,(uint64_t)j);
		else
			printf("Detected USB %s Controller on port %#:%#.%d\n", name, (uint64_t)i,(uint64_t)j,(uint32_t)k);
		
		if (strcmp(name,"UHCI")) {
			uint8_t rval = init_uhci_ctrlr(pci.BAR4&~3,pci.irq);
			if (rval) {
				printf("Initialized UHCI controller ID %d with %d ports IRQ %d.\n",(uint8_t)rval-1,get_uhci_controller(rval-1)->num_ports,pci.irq);
				ctrlrcounts[0]++;
			}
		} else if (strcmp(name,"xHCI")) {
			uint8_t rval = init_xhci_ctrlr(pci.BAR0&~15,pci.irq);
			if (rval) {
				printf("Initialized xHCI controller ID %d with %d ports IRQ %d.\n",(uint8_t)rval-1,get_xhci_controller(rval-1)->num_ports,pci.irq);
				ctrlrcounts[3]++;
			}
		}
	}
}

void usb_get_driver_for_class(uint16_t dev_addr, uint8_t class, uint8_t subclass, uint8_t protocol) {
	usb_config_desc config = usb_get_config_desc(dev_addr,0);
	usb_get_endpoint_desc(dev_addr,0,0,0);
	if (class==USB_CLASS_HUB)
		init_hub(dev_addr,config);
	if (class==USB_CLASS_HID)
		init_hid(dev_addr,config);
}

void init_usb() {
	pci_t c_pci;
	for (uint16_t i = 0; i < 256; i++) {
		c_pci = getPCIData(i,0,0);
		if (c_pci.vendorID!=0xFFFF) {
			for (uint8_t j = 0; j < 32; j++) {
				c_pci = getPCIData(i,j,0);
				if (c_pci.vendorID!=0xFFFF) {
					check_pci(c_pci,i,j,0);
					for (uint8_t k = 1; k < 8; k++) {
						pci_t pci = getPCIData(i,j,k);
						if (pci.vendorID!=0xFFFF) {
							check_pci(pci,i,j,k);
						}
					}
				}
			}
		}
	}
	kprint("[USB ] Assigning drivers to devices...");
	for (uint8_t ctype = 0; ctype < 1; ctype++) {
		for (uint8_t cid = 0; cid < ctrlrcounts[ctype]; cid++) {
			for (uint8_t dev_num = 1; dev_num < 128; dev_num++) {
				uint16_t devaddr = usb_dev_addr(ctype,cid,dev_num);
				usb_dev_desc devdesc = usb_get_dev_desc(devaddr);
				if (!devdesc.length)
					break;
				if (devdesc.dev_class) {
					usb_get_driver_for_class(devaddr,devdesc.dev_class,devdesc.dev_subclass,devdesc.dev_protocol);
				} else {
					usb_interface_desc interface = usb_get_interface_desc(devaddr,0,0);
					if (!interface.length)
						break;
					usb_get_driver_for_class(devaddr,interface.iclass,interface.isubclass,interface.iprotocol);
				}
			}
		}
	}
	kprint("[INIT] Initialized USB driver");
}

//	dev_addr:
//
//	0xF000 >> 12: Controller type
// 		0 = UHCI
//		1 = OHCI
//		2 = EHCI
//		3 = XHCI
//
//	0x0F00 >> 8: Controller ID
//		#   = Controller ID
//
//
//	0x007F	   :  Device ID 
//		0-127 = Device ID


uint16_t usb_dev_addr(uint8_t ctrlrtype, uint8_t ctrlrID, uint8_t devID) {
	return ((ctrlrtype&0x0F)<<12) | (ctrlrID<<8) | (devID&0x7F);
}

usb_device device_null = {0};

usb_device *usb_device_from_addr(uint16_t dev_addr) {
	if ((dev_addr&0xF000)>>12==USB_CTRLR_UHCI) {
		return &get_uhci_controller((dev_addr&0x0F00)>>8)->devices[dev_addr&0x007F];
	} else {
		return &device_null;
	}
}

bool usb_get_str_desc(uint16_t dev_addr, void *out, uint8_t index, uint16_t targetlang) {
	usb_device *device = usb_device_from_addr(dev_addr);
	if (device->controller&&!device->ctrlr_type) {
		return uhci_get_usb_str_desc(device,out,index,targetlang);
	} else {
		return false;
	}
}

usb_dev_desc usb_get_dev_desc(uint16_t dev_addr) {
	usb_device *device = usb_device_from_addr(dev_addr);
	if (device->controller&&!device->ctrlr_type) {
		return uhci_get_usb_dev_descriptor(device,sizeof(usb_dev_desc));
	} else {
		usb_dev_desc retnull;
		memset(&retnull,0,sizeof(usb_dev_desc));
		return retnull;
	}
}

usb_config_desc usb_get_config_desc(uint16_t dev_addr, uint8_t index) {
	usb_device *device = usb_device_from_addr(dev_addr);
	if (device->controller&&!device->ctrlr_type) {
		return uhci_get_config_desc(device,index);
	} else {
		usb_config_desc retnull;
		memset(&retnull,0,sizeof(usb_config_desc));
		return retnull;
	}
}

usb_interface_desc usb_get_interface_desc(uint16_t dev_addr, uint8_t config_index, uint8_t interface_index) {
	usb_device *device = usb_device_from_addr(dev_addr);
	if (device->controller&&!device->ctrlr_type) {
		return uhci_get_interface_desc(device, config_index, interface_index);
	} else {
		usb_interface_desc retnull;
		memset(&retnull,0,sizeof(usb_interface_desc));
		return retnull;
	}
}

usb_endpoint_desc usb_get_endpoint_desc(uint16_t dev_addr, uint8_t config_index, uint8_t interface_index, uint8_t endpoint_index) {
	usb_device *device = usb_device_from_addr(dev_addr);
	if (device->controller&&!device->ctrlr_type) {
		return uhci_get_endpoint_desc(device, config_index, interface_index, endpoint_index);
	} else {
		usb_endpoint_desc retnull;
		memset(&retnull,0,sizeof(usb_endpoint_desc));
		return retnull;
	}
}

bool usb_get_desc(uint16_t dev_addr, void *out, usb_setup_pkt setup_pkt_template, uint16_t size) {	
	usb_device *device = usb_device_from_addr(dev_addr);
	if (device->controller&&!device->ctrlr_type) {
		return uhci_usb_get_desc(device, out, setup_pkt_template, size);
	} else {
		return false;
	}
}

bool usb_generic_setup(uint16_t dev_addr, usb_setup_pkt setup_pkt_template) {
	usb_device *device = usb_device_from_addr(dev_addr);
	if (device->controller&&!device->ctrlr_type) {
		return uhci_generic_setup(device, setup_pkt_template);
	} else {
		return false;
	}
}

bool usb_assign_address(uint16_t port_addr, uint8_t lowspeed) {
	if ((port_addr&0xF000)>>12==USB_CTRLR_UHCI) {
		return uhci_assign_address((port_addr&0x0F00)>>8,port_addr&0xFF, lowspeed);
	} else {
		return false;
	}
}

void *usb_create_interval_in(uint16_t dev_addr, void *out, uint8_t interval, uint8_t endpoint_addr, uint16_t max_pkt_size, uint16_t size) {
	usb_device *device = usb_device_from_addr(dev_addr);
	if (device->controller&&!device->ctrlr_type) {
		return uhci_create_interval_in(device, out, interval, endpoint_addr, max_pkt_size, size);
	} else {
		return 0;
	}
}

bool usb_refresh_interval(uint16_t dev_addr, void *data) {
	usb_device *device = usb_device_from_addr(dev_addr);
	if (device->controller&&!device->ctrlr_type) {
		return uhci_refresh_interval(data);
	} else {
		return false;
	}
}

void dump_memory(uint8_t *mem, size_t size) {
	for (size_t i = 0; i < size; i++) {
		if (i&&!(i%16))
			dprintf("\n");
		if (mem[i]<0x10)
			dprintf("0");
		dprintf("%# ",(uint64_t)mem[i]);
	}
}