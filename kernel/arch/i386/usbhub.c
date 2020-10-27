#include <usb/hub.h>

bool hub_init_port(uint16_t dev_addr, uint8_t portnum, uint8_t potpgt) {
	usb_setup_pkt sp;
	sp.type = 0x23;
	sp.request = USB_SETUP_SETFEATURE;
	sp.value = HUB_FEATURE_PORTPOWER;
	sp.index = portnum+1;
	sp.length = 0;
	if (!usb_generic_setup(dev_addr,sp))
		return false;
	sleep(potpgt*2);
	sp.type = 0x23;
	sp.request = USB_SETUP_CLRFEATURE;
	sp.value = HUB_FEATURE_PORTCNCTN;
	sp.index = portnum+1;
	sp.length = 0;
	if (!usb_generic_setup(dev_addr,sp))
		return false;
	uint32_t status;
	sp.type = 0xA3;
	sp.request = USB_SETUP_GETSTATUS;
	sp.value = 0;
	sp.index = portnum+1;
	sp.length = 4;
	if (!usb_get_desc(dev_addr,&status,sp,sp.length))
		return false;
	if (!(status&HUB_STATUS_CONNECTION))
		return false;
	sp.type = 0x23;
	sp.request = USB_SETUP_SETFEATURE;
	sp.value = HUB_FEATURE_RESET;
	sp.index = portnum+1;
	sp.length = 0;
	if (!usb_generic_setup(dev_addr,sp))
		return false;
	sleep(50);
	sp.type = 0xA3;
	sp.request = USB_SETUP_GETSTATUS;
	sp.value = 0;
	sp.index = portnum+1;
	sp.length = 4;
	if (!usb_get_desc(dev_addr,&status,sp,sp.length))
		return false;
	if (!(status&HUB_STATUS_CONNECTION))
		return false;
	sp.type = 0x23;
	sp.request = USB_SETUP_CLRFEATURE;
	sp.value = HUB_FEATURE_CRESET;
	sp.index = portnum+1;
	sp.length = 0;
	if (!usb_generic_setup(dev_addr,sp))
		return false;
	printf("Hub found device!\n");
	return true;
}

bool init_hub(uint16_t dev_addr, usb_config_desc config) {
	printf("Initializing hub at %X\n",dev_addr);
	usb_device *device = usb_device_from_addr(dev_addr);
	usb_setup_pkt sp;
	sp.type = 0;
	sp.request = USB_SETUP_SETCONFIG;
	sp.value = config.config_value;
	sp.index = 0;
	sp.length = 0;
	if (!usb_generic_setup(dev_addr,sp))
		return false;
	usb_hub_desc hub;
	sp.type = 0xA0;
	sp.request = USB_SETUP_GETDESC;
	sp.value = 0x2900;
	sp.index = 0;
	sp.length = sizeof(usb_hub_desc);
	if (!usb_get_desc(dev_addr,&hub,sp,sp.length))
		return false;
	for (uint8_t i = 0; i < hub.num_ports; i++) {
		if (hub_init_port(dev_addr, i, hub.potpgt)) {
			uint32_t status;
			usb_setup_pkt sp;
			sp.type = 0xA3;
			sp.request = USB_SETUP_GETSTATUS;
			sp.value = 0;
			sp.index = i+1;
			sp.length = 4;
			if (!usb_get_desc(dev_addr,&status,sp,sp.length))
				break;
			usb_assign_address((dev_addr&0xFF00) | i, (status&HUB_STATUS_LS) ? 1 : 0);
		}
	}
	return true;
}