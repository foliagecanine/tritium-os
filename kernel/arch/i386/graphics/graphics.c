#include <kernel/graphics.h>

uint32_t graph_lock_pid = 0;

bool has_graphics_lock() {
	return graph_lock_pid == getpid();
}

uint32_t request_graphics_lock() {
	if (graph_lock_pid)
		return 1;
	graph_lock_pid = getpid();
	return 0;
}

uint32_t request_graphics_unlock() {
	if (has_graphics_lock())
		return 1;
	graph_lock_pid = 0;
	return 0;
}

extern void vesa32();
uint8_t error;
uint16_t framebuffer_width;
uint16_t framebuffer_height;
uint32_t framebuffer_pitch;
uint8_t framebuffer_bpp;
void *framebuffer_paddr;
void *framebuffer_addr;

uint32_t set_resolution(uint32_t width, uint32_t height, uint8_t bits) {
	if (!has_graphics_lock())
		return 1;
	if (bits != 16 && bits != 24 && bits != 32) // Disallow 15 bit mode so the calculations work
		return 2;
	identity_map_pages((paddr_t)0x7000, 2);
	framebuffer_width = width;
	framebuffer_height = height;
	framebuffer_bpp = bits;
	asm("\
		pusha;\
		mov framebuffer_width,%%eax;\
		mov framebuffer_height,%%ebx;\
		mov framebuffer_bpp,%%cl;\
		mov $0,%%ch;\
		call vesa32;\
		mov %%al,error;\
		mov %%ebx,framebuffer_paddr;\
		popa;\
	":::"eax","ebx","ecx");
	identity_free_pages((void *)0x7000, 2);
	init_pit(1000);
	if (error)
		return error;
	framebuffer_pitch = framebuffer_width * framebuffer_bpp / 8;
	framebuffer_addr = ioalloc_pages(framebuffer_paddr, ((framebuffer_height * framebuffer_pitch) + PAGE_SIZE - 1) / PAGE_SIZE);
	if (!framebuffer_addr)
		return 5;
	uint32_t retval = bits;
	retval |= ((height & 0xFFF) << 8);
	retval |= ((width & 0xFFF) << 20);
	return retval;
}

void fastmemcpy32(char *dst, char *src, size_t size) {
	for (size_t i = 0; i < size; i++) {
		dst[i] = src[i];
	}
}

uint32_t copy_framebuffer(void *addr) {
	if (!has_graphics_lock())
		return 1;
	if (!framebuffer_addr)
		return 2;
	if (!addr)
		return 3;
	fastmemcpy32(framebuffer_addr, addr, framebuffer_width*framebuffer_height*(framebuffer_bpp/8));
	return 0;
}

uint32_t get_framebuffer_size() {
	return framebuffer_pitch * framebuffer_height;
}
