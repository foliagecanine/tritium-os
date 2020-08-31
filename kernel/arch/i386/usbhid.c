#include <usb/hid.h>

bool init_hid_kbd(uint16_t dev_addr, usb_config_desc config, usb_interface_desc interface, usb_endpoint_desc endpoint, uint8_t endpoint_addr) {
	printf("Configuring HID Keyboard\n");
	usb_setup_pkt sp;
	sp.type = 0x21;
	sp.request = 0x0B;
	sp.value = 0;
	sp.index = 0;
	sp.length = 0;
	if (!usb_generic_setup(dev_addr,sp))
		return false;
}

bool init_hid_mouse(uint16_t dev_addr, usb_config_desc config, usb_interface_desc interface, usb_endpoint_desc endpoint, uint8_t endpoint_addr) {
	printf("Configuring HID Mouse\n");
	// Use boot protocol
	usb_setup_pkt sp;
	sp.type = 0x21;
	sp.request = 0x0B;
	sp.value = 0;
	sp.index = 0;
	sp.length = 0;
	if (!usb_generic_setup(dev_addr,sp))
		return false;
	
}

bool init_hid(uint16_t dev_addr, usb_config_desc config) {
	printf("Configuring HID device...\n");
	usb_setup_pkt sp;
	sp.type = 0x21;
	sp.request = 0x0A;
	sp.value = 0;
	sp.index = 0;
	sp.length = 0;
	if (!usb_generic_setup(dev_addr,sp))
		return false;
	usb_interface_desc interface = usb_get_interface_desc(dev_addr,0,0);
	if (!interface.length)
		return false;
	usb_endpoint_desc endpoint;
	uint8_t endpoint_addr = 0xFF;
	for (uint8_t i = 0; i < interface.num_endpoints; i++) {
		endpoint = usb_get_endpoint_desc(dev_addr,0,0,i);
		if (!endpoint.length)
			return false;
		if ((endpoint.attributes&3)==3 && endpoint.endpoint_addr&0x80) {
			endpoint_addr = endpoint.endpoint_addr&0xF;
			break;
		}
	}
	if (endpoint_addr==0xFF)
		return false;
	printf("Found endpoint!\n");
	if (interface.iprotocol==1)
		return init_hid_kbd(dev_addr,config,interface,endpoint,endpoint_addr);
	else if (interface.iprotocol==2)
		return init_hid_mouse(dev_addr,config,interface,endpoint,endpoint_addr);
	return false;
}