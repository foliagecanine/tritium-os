#include <fs/disk.h>

// Codes:
// 5 = Invalid Filesystem
// 8+ = Drive error = (error_code - 8) (see ahci.c, ide.c)

typedef struct {
	bool used;
	uint8_t (*read_function)(uint16_t drive_num, uint64_t start_sector, uint32_t count, void *buf);
	uint8_t (*write_function)(uint16_t drive_num, uint64_t start_sector, uint32_t count, void *buf);
} disk_handler;

// Save space for 255 drivers to register. Please don't write more than 255 drivers for accessing storage media...
disk_handler handlers[255] = {0};

uint8_t register_disk_handler(uint8_t (*read_function)(uint16_t, uint64_t, uint32_t, void*), uint8_t (*write_function)(uint16_t, uint64_t, uint32_t, void*)) {
	for (uint16_t i = 0; i < 255; i++) {
		if (!handlers[i].used) {
			handlers[i].used = true;
			handlers[i].read_function = read_function;
			handlers[i].write_function = write_function;
			return i;
		}
	}
	return 255; // 255 is error
}

void unregister_disk_handler(uint8_t handler_id) {
	if (handler_id == 255)
		return;
	handlers[handler_id].used = false;
	handlers[handler_id].read_function = NULL;
	handlers[handler_id].write_function = NULL;
}

uint8_t disk_read_sectors(uint32_t drive_num, uint64_t start_sector, uint32_t count, void *buf) {
	uint8_t	handler = (uint8_t)(drive_num >> 16);
	if (handler == 255)
		return 5;
	if (!handlers[handler].used)
		return 5;
	return handlers[handler].read_function(drive_num, start_sector, count, buf);
}

uint8_t disk_write_sectors(uint32_t drive_num, uint64_t start_sector, uint32_t count, void *buf) {
	uint8_t	handler = (uint8_t)(drive_num >> 16);
	if (handler == 255)
		return 5;
	if (!handlers[handler].used)
		return 5;
	return handlers[handler].write_function(drive_num, start_sector, count, buf);
}
