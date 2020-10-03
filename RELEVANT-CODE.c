/*
 * xhci.c:280
 */
 
const uint16_t default_speeds[] = {64,8,64,512};
 
bool xhci_init_port_dev(xhci_controller *xc, uint8_t port) {
	void *portbase = xc->hcops+XHCI_HCOPS_PORTREGS+(port*16);
	xhci_trb trb;
	trb.param = 0;
	trb.status = 0;
	trb.command = XHCI_TRB_TRBTYPE(XHCI_TRBTYPE_ENSLOT);
	
	trb = xhci_send_cmdtrb(xc,trb);
	if (XHCI_EVTTRB_STATUS_GETCODE(trb.status)!=XHCI_TRBCODE_SUCCESS)
		return false;
	
	usb_device *usbdev = &xc->devices[0];
	memset(usbdev,0,sizeof(usb_device));
	usbdev->slot = XHCI_EVTTRB_COMMAND_GETSLOT(trb.command);
	usbdev->speed = XHCI_PORTREGS_PORTSC_PORTSPD(_rd32(portbase+XHCI_PORTREGS_PORTSC));
	usbdev->controller = xc;
	usbdev->ctrlrID = xc->ctrlrID;
	usbdev->ctrlr_type = USB_CTRLR_XHCI;
	usbdev->max_pkt_size = default_speeds[usbdev->speed];
	usbdev->data = alloc_page(1);
	xhci_devdata *data = usbdev->data;
	
	xhci_init_device(usbdev,xc);
	data->slot_template = alloc_page(1);
	memset(data->slot_template,0,4096);
	memcpy((void *)(data->slot_template)+xc->ctx_size,data->slot_ctx,2048);
	((uint32_t *)data->slot_template)[1] = 3;
	//xhci_set_address(usbdev,0);
	if (!xhci_set_address(usbdev,0))
		return false;
	
	usbdev->address = data->slot_ctx->devaddr;
	memcpy(&xc->devices[usbdev->address],usbdev,sizeof(usb_device));
	usbdev = &xc->devices[usbdev->address];
	usbdev->valid = true;
	
	usb_dev_desc devdesc = xhci_get_usb_dev_descriptor(usbdev,8);
	
	return true;
}

/*
 * xhci.c:238
 */
 
void *xhci_init_device(usb_device *usbdev, xhci_controller *xc) {
	xhci_devdata *data = usbdev->data;
	data->slot_ctx = alloc_page(1);
	memset(data->slot_ctx,0,4096);
	xhci_slot *this_slot = data->slot_ctx;
	*(uint64_t *)(xc->dcbaap+(usbdev->slot*8)) = (uint64_t)(uint32_t)(void *)this_slot;
	
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
	
	data->endpt_ring[0] = alloc_page(1);
	data->cendpttrb[0] = data->endpt_ring[0];
	ep.dqptr = (uint64_t)(uint32_t)get_phys_addr(data->endpt_ring[0]) | XHCI_TRB_CYCLE;
	memset(data->endpt_ring[0],0,4096);
	data->endpt_ring[0][127].param = ep.dqptr;
	data->endpt_ring[0][127].status = 0;
	data->endpt_ring[0][127].command = XHCI_TRB_TRBTYPE(XHCI_TRBTYPE_LINK) | XHCI_TRB_CYCLE;
	data->endpt_cycle[0] = XHCI_TRB_CYCLE;
	
	xhci_endpt *this_ep = ((void *)data->slot_ctx)+xc->ctx_size;
	*this_ep = ep;
	
	xc->dcbaap[usbdev->slot] = get_phys_addr(data->slot_ctx);
	
	return data->slot_ctx;
}

/*
 * xhci.c:664
 */
 
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

/*
 * xhci.c:633
 */
 
bool xhci_usb_get_desc(usb_device *device, void *out, usb_setup_pkt setup_pkt_template, uint16_t size) {	
	xhci_controller *xc = device->controller;
	void *data = alloc_page(((size+sizeof(uint32_t))/4096)+1);
	uint32_t *status = data;
	void *buffer = data+sizeof(uint32_t);
	
	xhci_setup_trb(device, setup_pkt_template,XHCI_DIRECTION_IN);
	xhci_data_trbs(device, buffer, status, size, XHCI_DATA_DIR_IN);
	ring_doorbell(xc,device->slot,1);
	
	status = xhci_await_int(status);
	
	if (XHCI_EVTTRB_STATUS_GETCODE(*status)==XHCI_TRBCODE_SUCCESS)
		return true;
	
	dbgprintf("[xHCI] Error: Timed out or bad code\n");
	return false;
}

/*
 * xhci.c:497
 */

void xhci_setup_trb(usb_device *device, usb_setup_pkt setup_pkt, uint8_t direction) {
	xhci_devdata *data = device->data;
	
	xhci_trb trb;
	trb.param = *(uint64_t *)&setup_pkt; // Convert setup packet into uint64_t
	trb.status = 8;
	trb.command = XHCI_TRB_SETUP_DIRECTION(direction) | XHCI_TRB_TRBTYPE(XHCI_TRBTYPE_SETUP) | XHCI_TRB_IMMEDIATE | data->endpt_cycle[0];
	
	*data->cendpttrb[0] = trb;
	
	dbgprintf("[xHCI] DATA: Send SETUP TRB\n");
	dbgprintf("[xHCI] DATA: Param: %#\n",trb.param);
	dbgprintf("[xHCI] DATA: Status: %#\n",(uint64_t)trb.status);
	dbgprintf("[xHCI] DATA: Command: %#\n",(uint64_t)trb.command);
	
	xhci_advance_trb(data->endpt_ring[0],&data->cendpttrb[0],&data->endpt_cycle[0]);
	
	dbgprintf("[xHCI] DATA: New CENDPTTRB: %#\n",(uint64_t)(uint32_t)data->cendpttrb[0]);
}

void xhci_data_trbs(usb_device *device, void *buffer, uint32_t *status, uint16_t size, uint8_t direction) {
	void *current_buffer = buffer;
	xhci_devdata *data = device->data;
	uint16_t size_remaining = size;
	uint8_t trbs_remaining = ((size + (device->max_pkt_size-1)) / device->max_pkt_size)-1;
	uint8_t trb_type = XHCI_TRBTYPE_DATA;
	
	while (size_remaining) {
		xhci_trb trb;
		trb.param = (uint64_t)(uint32_t)get_phys_addr(current_buffer);
		trb.status = XHCI_TRB_DATA_REMAINING(trbs_remaining) | ((size_remaining < device->max_pkt_size) ? size_remaining : device->max_pkt_size);
		trb.command = XHCI_TRB_DATA_DIRECTION(direction) | XHCI_TRB_TRBTYPE(trb_type) | XHCI_TRB_CHAIN | (trbs_remaining?0:XHCI_TRB_EVALTRB) | data->endpt_cycle[0];
		*data->cendpttrb[0] = trb;
		
		dbgprintf("[xHCI] DATA: TRBs remain: %d\n",trbs_remaining);
		dbgprintf("[xHCI] DATA: Send DATA TRB\n");
		dbgprintf("[xHCI] DATA: Param: %#\n",trb.param);
		dbgprintf("[xHCI] DATA: Status: %#\n",(uint64_t)trb.status);
		dbgprintf("[xHCI] DATA: Command: %#\n",(uint64_t)trb.command);
		
		xhci_advance_trb(data->endpt_ring[0],&data->cendpttrb[0],&data->endpt_cycle[0]);
		
		dbgprintf("[xHCI] DATA: New CENDPTTRB: %#\n",(uint64_t)(uint32_t)data->cendpttrb[0]);
		
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
	trb.command = XHCI_TRB_TRBTYPE(XHCI_TRBTYPE_EVENT) | XHCI_TRB_IOC | data->endpt_cycle[0];
	*data->cendpttrb[0] = trb;
	
	dbgprintf("[xHCI] DATA: Send EVENT TRB\n");
	dbgprintf("[xHCI] DATA: Param: %#\n",trb.param);
	dbgprintf("[xHCI] DATA: Status: %#\n",(uint64_t)trb.status);
	dbgprintf("[xHCI] DATA: Command: %#\n",(uint64_t)trb.command);
	
	xhci_advance_trb(data->endpt_ring[0],&data->cendpttrb[0],&data->endpt_cycle[0]);
	
	dbgprintf("[xHCI] DATA: New CENDPTTRB: %#\n",(uint64_t)(uint32_t)data->cendpttrb[0]);
}

/*
 * xhci.c:439
 */
 
void xhci_advance_trb(xhci_trb *ring_start, xhci_trb **current_pointer, uint8_t *current_cycle) {
	(*current_pointer)++;
	
	if (XHCI_TRB_GTRBTYPE((*current_pointer)->command)==XHCI_TRBTYPE_LINK) {
		(*current_pointer)->command = ((*current_pointer)->command & ~1) | *current_cycle;
		*current_cycle ^= 1;
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
	
	return status;
}

/*
 * xhci.c:51
 */
 
void ring_doorbell(xhci_controller* xc, uint8_t slot, uint32_t val) {
	dbgprintf("[xHCI] Ringing doorbell %d with value %d\n",(uint32_t)slot,val);
	_wr32(xc->dboff+(slot*sizeof(uint32_t)),val);
}

/*
 * xhci.c:206
 */

void xhci_interrupt() {
	dbgprintf("[xHCI] USB INTERRUPT\n");
	for (uint8_t i = 0; i < USB_MAX_CTRLRS; i++) {
		xhci_controller *xc = get_xhci_controller(i);
		if (!xc->hcops)
			continue;
		if (!_rd32(xc->hcops+XHCI_HCOPS_USBSTS))
			continue;
		_wr32(xc->hcops+XHCI_HCOPS_USBSTS,_rd32(xc->hcops+XHCI_HCOPS_USBSTS));
		if ((_rd32(xc->runtime+XHCI_RUNTIME_IR0+XHCI_INTREG_IMR)&(XHCI_INTREG_IMR_EN|XHCI_INTREG_IMR_IP))==(XHCI_INTREG_IMR_EN|XHCI_INTREG_IMR_IP)) {
			_wr32(xc->runtime+XHCI_RUNTIME_IR0+XHCI_INTREG_IMR,_rd32(xc->runtime+XHCI_RUNTIME_IR0+XHCI_INTREG_IMR));
			while ((xc->cevttrb->command&XHCI_TRB_CYCLE)==xc->evtcycle) {
				if (xc->cevttrb->command != 0x8801) {
					xhci_trb *exec_trb = (xc->cevttrb->param&0xFFF)+((void *)xc->cmdring);
					exec_trb->param = (uint64_t)(uint32_t)(void *)xc->cevttrb;
				}
				dbgprintf("[xHCI] DATA: Processed TRB\n");
				dbgprintf("[xHCI] DATA: Param: %#\n",xc->cevttrb->param);
				dbgprintf("[xHCI] DATA: Status: %#\n",(uint64_t)xc->cevttrb->status);
				dbgprintf("[xHCI] DATA: Command: %#\n",(uint64_t)xc->cevttrb->command);
				xc->cevttrb++;
				if ((void *)(xc->cevttrb)>(void *)xc->evtring+65536) {
					xc->cevttrb = xc->evtring;
					xc->evtcycle ^= 1;
				}
			}
			dbgprintf("[xHCI] DATA: New ERDQPTR: %#\n",(uint64_t)(uint32_t)xc->cevttrb);
			_wr64(xc->runtime+XHCI_RUNTIME_IR0+XHCI_INTREG_ERDQPTR,((uint64_t)(uint32_t)get_phys_addr(xc->cevttrb))|XHCI_INTREG_ERDQPTR_EHBSY,xc);
		}
	}
}
