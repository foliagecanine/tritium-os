#include <fs/ide.h>

/* Error codes:
 *
 * 1 : Invalid drive
 * 2 : Timed out
 */

#define IDE_CMD_READ	0x20
#define IDE_CMD_WRITE	0x30

#define IDE_BSY_FLAG	0x80

uint8_t ide_read_sectors(uint8_t drive_num, uint32_t start_sector, uint32_t count, uint8_t *buf) {
	uint16_t iobase;
	uint8_t drive = 0x40;

	switch (drive_num) {
		case 0:
		case 1:
			iobase = 0x1f0;
			break;
		case 2:
		case 3:
			iobase = 0x170;
			break;
		default:
			return 1;
	}

	drive |= (drive_num % 2) << 4;

	outb(iobase + 2, count);
	outb(iobase + 3, (uint8_t)((start_sector & 0x000000FF)      ));
	outb(iobase + 4, (uint8_t)((start_sector & 0x0000FF00) >>  8));
    outb(iobase + 5, (uint8_t)((start_sector & 0x00FF0000) >> 16));
    outb(iobase + 6, (uint8_t)((start_sector & 0x0F000000) >> 24) | drive);
	outb(iobase + 7, IDE_CMD_READ);

	uint16_t spin = 10000;
	while((inb(iobase + 7) & IDE_BSY_FLAG) && spin--)
		;

	if (spin == 0)
		return 2;

	uint16_t *buffer = (uint16_t *)buf;
	for (uint32_t remain = count * 512 / 2; remain != 0; remain--) {
		*buffer++ = inw(iobase);
	}

	return 0;
}

uint8_t ide_write_sectors(uint8_t drive_num, uint32_t start_sector, uint32_t count, uint8_t *buf) {
	uint16_t iobase;
	uint8_t drive = 0x40;

	switch (drive_num) {
		case 0:
		case 1:
			iobase = 0x1f0;
			break;
		case 2:
		case 3:
			iobase = 0x170;
			break;
		default:
			return 1;
	}

	drive |= (drive_num % 2) << 4;

	outb(iobase + 2, count);
	outb(iobase + 3, (uint8_t)((start_sector & 0x000000FF)      ));
	outb(iobase + 4, (uint8_t)((start_sector & 0x0000FF00) >>  8));
    outb(iobase + 5, (uint8_t)((start_sector & 0x00FF0000) >> 16));
    outb(iobase + 6, (uint8_t)((start_sector & 0x0F000000) >> 24) | drive);
	outb(iobase + 7, IDE_CMD_WRITE);

	uint16_t spin = 10000;
	while((inb(iobase + 7) & IDE_BSY_FLAG) && spin--)
		;

	if (spin == 0)
		return 2;

	uint16_t *buffer = (uint16_t *)buf;
	for (uint32_t remain = count * 512 / 2; remain != 0; remain--) {
		outw(iobase, *buffer++);
	}

	return 0;
}
