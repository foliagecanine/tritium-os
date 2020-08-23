#include <usb/usb.h>

char name[5];

void check_pci(pci_t pci, uint16_t i, uint8_t j, uint8_t k) {
	if (pci.vendorID!=0xFFFF&&pci.classCode==0xC&&pci.subclass==3) {
		if (pci.progIF==0x00)
			strcpy(name,"UHCI");
		else if (pci.progIF==0x10)
			strcpy(name,"OHCI");
		else if (pci.progIF==0x20)
			strcpy(name,"EHCI");
		else if (pci.progIF==0x30)
			strcpy(name,"XHCI");
		else
			strcpy(name,"ERR!");
		if (k==0)
			printf("Detected USB %s Controller on port %#:%#\n", name, (uint64_t)i,(uint64_t)j);
		else
			printf("Detected USB %s Controller on port %#:%#.%d\n", name, (uint64_t)i,(uint64_t)j,(uint32_t)k);
		
		if (strcmp(name,"UHCI")) {
			bool rval = init_uhci_ctrlr(pci.BAR4&~3);
			if (rval) {
				printf("Initialized UHCI controller ID %d with %d ports.\n",(uint8_t)rval-1,get_uhci_controller(rval-1).num_ports);
			}
		}
	}
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
	kprint("[INIT] Initialized USB driver");
}

//	dev_addr:
//
//	0x0F000000 >> 24: Controller type
// 		0 = UHCI
//		1 = OHCI
//		2 = EHCI
//		3 = XHCI
//
//	0x00FF0000 >> 16: Controller ID
//		#   = Controller ID
//
//	0x0000FF00 >> 8:  Port ID
//		#   = Root Port ID
//
//	0x0000007F	   :  Device ID 
//		0-127 = Device ID


uint32_t usb_dev_addr(uint8_t ctrlrtype, uint8_t ctrlrID, uint8_t portID, uint8_t devID) {
	return ((ctrlrtype&0x0F)<<24) | (ctrlrID<<16) | (portID<<8) | (devID&0x7F);
}

usb_dev_desc get_usb_dev_desc(uint32_t dev_addr) {
	if (!(dev_addr&0x0F000000)) {
		dprintf("Attempting to get USB Device Descriptor from UHCI %d.%d.%d\n",(uint32_t)((dev_addr&0x00FF0000)>>16),(uint32_t)((dev_addr&0x0000FF00)>>8),(uint32_t)(dev_addr&0x7F));
		return uhci_get_usb_descriptor(get_uhci_controller((dev_addr&0x00FF0000)>>16),dev_addr&0x7F,(dev_addr&0x0000FF00)>>8,8,8);
	} else {
		usb_dev_desc retnull;
		memset(&retnull,0,sizeof(usb_dev_desc));
		return retnull;
	}
}