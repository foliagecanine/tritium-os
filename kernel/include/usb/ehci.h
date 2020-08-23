#ifndef _USB_EHCI_H
#define _USB_EHCI_H

#include <kernel/stdio.h>

typedef struct {
	uint8_t cap_len;
	uint8_t reserved0;
	uint16_t hciver;
	uint32_t hcsparams;
	uint32_t hccparams;
	uint64_t hcsp_portroute;
} __attribute__((packed)) usb_ehci_cap_regs;

typedef struct {
	uint32_t usb_cmd;
	uint32_t usb_status;
	uint32_t usb_int_enable;
	uint32_t usb_frame_index;
	uint32_t ctrl_ds_selector;
	uint32_t periodic_list_table;
	uint32_t async_list_addr;
	uint8_t reserved0[36];
	uint32_t config_flag;
	uint32_t portsc[1];
} __attribute__((packed)) usb_ehci_op_regs;

typedef struct {
	uint8_t run:1;
	uint8_t host_ctrl_reset:1;
	uint8_t prgm_frame_list_size:2;
	uint8_t periodic_sched_enable:1;
	uint8_t async_sched_enable:1;
	uint8_t int_on_async_adv_doorbell:1;
	uint8_t light_host_ctrl_reset:1;
	uint8_t async_sched_park_mode_cnt:2;
	uint8_t reserved0:1;
	uint8_t async_sched_park_mode_enable:1;
	uint8_t reserved1:4;
	uint8_t int_threshold;
	uint8_t reserved2;
} __attribute__((packed)) usb_ehci_cmd_reg;

typedef struct {
	uint8_t usb_xfr_int:1;
	uint8_t usb_err_int:1;
	uint8_t port_chg_detect:1;
	uint8_t frame_list_rollover:1;
	uint8_t host_sys_err:1;
	uint8_t doorbell_int:1;
	uint8_t reserved0:2;
	uint8_t reserved1:4;
	uint8_t halted:1;
	uint8_t reclamation:1;
	uint8_t periodic_sched_status:1;
	uint8_t async_sched_status:1;
	uint16_t reserved2;
} __attribute__((packed)) usb_ehci_status_reg;

typedef struct {
	uint8_t usb_xfr_int_enable:1;
	uint8_t usb_err_int_enable:1;
	uint8_t port_chg_int_enable:1;
	uint8_t frame_list_rollover_int_enable:1;
	uint8_t host_sys_err_int_enable:1;
	uint8_t async_adv_int_enable:1;
	uint8_t reserved0:2;
	uint16_t reserved1;
} __attribute__((packed)) usb_ehci_int_enable_reg;

typedef struct {
	uint8_t connected:1;
	uint8_t connect_chg:1;
	uint8_t port_enabled:1;
	uint8_t port_enabled_chg:1;
	uint8_t overcurrent:1;
	uint8_t overcurrent_chg:1;
	uint8_t force_port_resume:1;
	uint8_t suspend:1;
	uint8_t port_reset:1;
	uint8_t reserved0:1;
	uint8_t line_status:2;
	uint8_t port_pwr:1;
	uint8_t companion_port_ctl:1;
	uint8_t port_indicator_ctl:2;
	uint8_t port_test_ctl:4;
	uint8_t wake_on_connect_enable:1;
	uint8_t wake_on_disconnect_enable:1;
	uint8_t wake_on_overcurrent_enable:1;
	uint8_t reserved1:1;
	uint8_t reserved2;
} __attribute__((packed)) usb_ehci_port_status_ctl_reg;

#endif