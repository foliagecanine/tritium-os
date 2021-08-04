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
	identity_map((void *)0x7000);
	identity_map((void *)0x8000);
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
	free_page((void *)0x7000,2);
	init_pit(1000);
	if (error)
		return error;
	framebuffer_pitch = framebuffer_width * framebuffer_bpp / 8;
	framebuffer_addr = map_paddr(framebuffer_paddr, ((framebuffer_height * framebuffer_pitch) / 4096) + 1);
	if (!framebuffer_addr)
		return 5;
	uint32_t retval = bits;
	retval |= ((height & 0xFFF) << 8);
	retval |= ((width & 0xFFF) << 20);
	return retval;
}

uint32_t _size;
uint32_t _dst;
uint32_t _src;

void fastmemcpy32(uint32_t *dst, uint32_t *src, size_t size) {
	_size=size/4;
	_dst=(uint32_t)dst;
	_src=(uint32_t)src;
	asm("\
	mov _dst,%%edi;\
	mov _src,%%esi;\
	mov _size,%%ecx;\
	cld;\
	rep movsd;\
	":::"edi","esi","ecx","memory");
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