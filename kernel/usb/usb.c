#include <usb/usb.h>

char name[5];

uint8_t ctrlrcounts[4] = {0, 0, 0, 0};

void usb_get_driver_for_class(uint16_t dev_addr, uint8_t class, uint8_t subclass, uint8_t protocol) {
	(void)subclass; // These two will be used later when we get more drivers
	(void)protocol;
	usb_config_desc config = usb_get_config_desc(dev_addr, 0);
	// usb_get_endpoint_desc(dev_addr,0,0,0);
	if (class == USB_CLASS_HUB)
		init_hub(dev_addr, config);
	else if (class == USB_CLASS_HID)
		init_hid(dev_addr, config);
	else
		printf("No driver for class %u subclass %u protocol %u\n");
}

void check_pci(pci_t pci, uint8_t i, uint8_t j, uint8_t k) {
	(void)i;
	(void)j;
	(void)k;
	if (pci.vendorID != 0xFFFF && pci.class == 0xC && pci.subclass == 3) {
		if (pci.progIF == 0x00)
			strcpy(name, "UHCI");
		else if (pci.progIF == 0x10)
			strcpy(name, "OHCI");
		else if (pci.progIF == 0x20)
			strcpy(name, "EHCI");
		else if (pci.progIF == 0x30)
			strcpy(name, "xHCI");
		else
			strcpy(name, "ERR!");

		/*if (k == 0)
		  printf("Detected USB %s Controller on port %X:%X\n", name, i, j);
		else
		  printf("Detected USB %s Controller on port %X:%X.%u\n", name, i, j, k);*/

		uint8_t rval = 0;
		uint8_t ctype;

		if (strcmp(name, "UHCI")) {
			ctype = USB_CTRLR_UHCI;
			rval = init_uhci_ctrlr(pci.BAR4 & ~3, pci.irq);
			if (rval) {
				printf("[USB ] Initialized UHCI controller ID %u with %u ports IRQ %u.\n", (uint8_t)rval - 1, get_uhci_controller(rval - 1)->num_ports, pci.irq);
				ctrlrcounts[USB_CTRLR_UHCI]++;
			}
		} else if (strcmp(name, "xHCI")) {
			ctype = USB_CTRLR_XHCI;
			rval = init_xhci_ctrlr((void *)(pci.BAR0 & ~15), pci.irq);
			if (rval) {
				printf("[USB ] Initialized xHCI controller ID %u with %u ports IRQ %u.\n", (uint8_t)rval - 1, get_xhci_controller(rval - 1)->num_ports, pci.irq);
				ctrlrcounts[USB_CTRLR_XHCI]++;
			}
		}

		if (!rval)
			return;

		for (uint8_t dev_num = 1; dev_num < 128; dev_num++) {
			uint16_t devaddr = usb_dev_addr(ctype, ctrlrcounts[ctype] - 1, dev_num);
			if (!usb_device_from_addr(devaddr)->valid)
				continue;
			usb_dev_desc devdesc = usb_get_dev_desc(devaddr);
			if (!devdesc.length)
				continue;
			if (devdesc.dev_class) {
				usb_get_driver_for_class(devaddr, devdesc.dev_class, devdesc.dev_subclass, devdesc.dev_protocol);
			} else {
				usb_interface_desc interface = usb_get_interface_desc(devaddr, 0, 0);
				if (!interface.length)
					continue;
				usb_get_driver_for_class(devaddr, interface.iclass, interface.isubclass, interface.iprotocol);
			}
		}
	}
}

void init_usb() {
	register_pci_driver(check_pci, 0xC, 3);
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

uint16_t usb_dev_addr(uint8_t ctrlrtype, uint8_t ctrlrID, uint8_t devID) { return ((ctrlrtype & 0x0F) << 12) | (ctrlrID << 8) | (devID & 0x7F); }

usb_device device_null = {0};

usb_device *usb_device_from_addr(uint16_t dev_addr) {
	uint8_t ctrlrtype = (dev_addr & 0xF000) >> 12;
	uint8_t ctrlrID = (dev_addr & 0x0F00) >> 8;
	uint8_t devID = dev_addr & 0x007F;
	switch (ctrlrtype) {
	case USB_CTRLR_UHCI:
		return &get_uhci_controller(ctrlrID)->devices[devID];
	case USB_CTRLR_XHCI:
		return &get_xhci_controller(ctrlrID)->devices[devID];
	default:
		return &device_null;
	}
}

bool usb_get_str_desc(uint16_t dev_addr, void *out, uint8_t index, uint16_t targetlang) {
	usb_device *device = usb_device_from_addr(dev_addr);
	if (device->valid) {
		switch (device->ctrlr_type) {
		case USB_CTRLR_UHCI:
			return uhci_get_usb_str_desc(device, out, index, targetlang);
		case USB_CTRLR_XHCI:
			return xhci_get_usb_str_desc(device, out, index, targetlang);
		default:
			break;
		}
	}
	return false;
}

usb_dev_desc usb_get_dev_desc(uint16_t dev_addr) {
	usb_device *device = usb_device_from_addr(dev_addr);
	if (device->valid) {
		switch (device->ctrlr_type) {
		case USB_CTRLR_UHCI:
			return uhci_get_usb_dev_descriptor(device, sizeof(usb_dev_desc));
		case USB_CTRLR_XHCI:
			return xhci_get_usb_dev_descriptor(device, sizeof(usb_dev_desc));
		default:
			break;
		}
	}
	usb_dev_desc retnull;
	memset(&retnull, 0, sizeof(usb_dev_desc));
	return retnull;
}

usb_config_desc usb_get_config_desc(uint16_t dev_addr, uint8_t index) {
	usb_device *device = usb_device_from_addr(dev_addr);
	if (device->valid) {
		switch (device->ctrlr_type) {
		case USB_CTRLR_UHCI:
			return uhci_get_config_desc(device, index);
		case USB_CTRLR_XHCI:
			return xhci_get_config_desc(device, index);
		default:
			break;
		}
	}
	usb_config_desc retnull;
	memset(&retnull, 0, sizeof(usb_config_desc));
	return retnull;
}

usb_interface_desc usb_get_interface_desc(uint16_t dev_addr, uint8_t config_index, uint8_t interface_index) {
	usb_device *device = usb_device_from_addr(dev_addr);
	if (device->valid) {
		switch (device->ctrlr_type) {
		case USB_CTRLR_UHCI:
			return uhci_get_interface_desc(device, config_index, interface_index);
		case USB_CTRLR_XHCI:
			return xhci_get_interface_desc(device, config_index, interface_index);
		default:
			break;
		}
	}
	usb_interface_desc retnull;
	memset(&retnull, 0, sizeof(usb_interface_desc));
	return retnull;
}

usb_endpoint_desc usb_get_endpoint_desc(uint16_t dev_addr, uint8_t config_index, uint8_t interface_index, uint8_t endpoint_index) {
	usb_device *device = usb_device_from_addr(dev_addr);
	if (device->valid) {
		switch (device->ctrlr_type) {
		case USB_CTRLR_UHCI:
			return uhci_get_endpoint_desc(device, config_index, interface_index, endpoint_index);
		case USB_CTRLR_XHCI:
			return xhci_get_endpoint_desc(device, config_index, interface_index, endpoint_index);
		default:
			break;
		}
	}
	usb_endpoint_desc retnull;
	memset(&retnull, 0, sizeof(usb_endpoint_desc));
	return retnull;
}

bool usb_enable_endpoint(uint16_t dev_addr, uint8_t endpoint, uint8_t flags, uint8_t interval) {
	usb_device *device = usb_device_from_addr(dev_addr);
	if (device->valid) {
		switch (device->ctrlr_type) {
		case USB_CTRLR_UHCI:
			// UHCI does not require endpoints to be enabled.
			return true;
		case USB_CTRLR_XHCI:
			return xhci_enable_endpoint(device, endpoint, flags, interval);
		default:
			break;
		}
	}
	return false;
}

bool usb_get_desc(uint16_t dev_addr, void *out, usb_setup_pkt setup_pkt_template, uint16_t size) {
	usb_device *device = usb_device_from_addr(dev_addr);
	if (device->valid) {
		switch (device->ctrlr_type) {
		case USB_CTRLR_UHCI:
			uhci_usb_get_desc(device, out, setup_pkt_template, size);
			return true;
		case USB_CTRLR_XHCI:
			xhci_usb_get_desc(device, out, setup_pkt_template, size);
			return true;
		default:
			break;
		}
	}
	return false;
}

bool usb_generic_setup(uint16_t dev_addr, usb_setup_pkt setup_pkt_template) {
	usb_device *device = usb_device_from_addr(dev_addr);
	if (device->valid) {
		switch (device->ctrlr_type) {
		case USB_CTRLR_UHCI:
			uhci_generic_setup(device, setup_pkt_template);
			return true;
		case USB_CTRLR_XHCI:
			xhci_generic_setup(device, setup_pkt_template);
			return true;
		default:
			break;
		}
	}
	return false;
}

bool usb_register_hub(uint16_t dev_addr) {
	usb_device *device = usb_device_from_addr(dev_addr);
	if (device->valid) {
		switch (device->ctrlr_type) {
		case USB_CTRLR_UHCI:
			return true;
		case USB_CTRLR_XHCI:
			return xhci_register_hub(device);
		default:
			break;
		}
	}
	return false;
}

bool usb_assign_address(uint16_t parentaddr, uint8_t port, uint8_t speed) {
	switch ((parentaddr & 0xF000) >> 12) {
	case USB_CTRLR_UHCI:
		return uhci_assign_address(parentaddr, port, speed);
	case USB_CTRLR_XHCI:
		return xhci_assign_address(parentaddr, port, speed);
	}
	return false;
}

void *usb_create_interval_in(uint16_t dev_addr, void *out, uint8_t interval, uint8_t endpoint_addr, uint16_t max_pkt_size, uint16_t size) {
	usb_device *device = usb_device_from_addr(dev_addr);
	if (device->valid) {
		switch (device->ctrlr_type) {
		case USB_CTRLR_UHCI:
			return uhci_create_interval_in(device, out, interval, endpoint_addr, max_pkt_size, size);
		case USB_CTRLR_XHCI:
			return xhci_create_interval_in(device, out, interval, endpoint_addr, max_pkt_size, size);
		default:
			break;
		}
	}
	return 0;
}

bool usb_refresh_interval(uint16_t dev_addr, void *data) {
	usb_device *device = usb_device_from_addr(dev_addr);
	if (device->valid) {
		switch (device->ctrlr_type) {
		case USB_CTRLR_UHCI:
			return uhci_refresh_interval(data);
		case USB_CTRLR_XHCI:
			return xhci_refresh_interval(data);
		default:
			break;
		}
	}
	return false;
}

void dump_memory(uint8_t *mem, size_t size) {
	for (size_t i = 0; i < size; i++) {
		if (i && !(i % 16))
			dprintf("\n");
		if (mem[i] < 0x10)
			dprintf("0");
		dprintf("%# ", (uint64_t)mem[i]);
	}
}

void dump_memory32(uint32_t *mem, size_t size) {
	for (size_t i = 0; i < size; i++) {
		if (i && !(i % 8))
			dprintf("\n");
		for (uint8_t j = 0; j < 7; j++) {
			if ((mem[i] << (j * 4)) & 0xF0000000)
				break;
			dprintf("0");
		}
		dprintf("%# ", (uint64_t)mem[i]);
	}
}
