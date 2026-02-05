#include <kernel/graphics.h>
#include <kernel/tty.h>
#include <kernel/kbd.h>

uint32_t graph_lock_pid = 0;

bool has_graphics_lock(uint32_t pid) {
	return graph_lock_pid == pid;
}

uint32_t request_graphics_lock() {
	if (graph_lock_pid)
		return GRAPHICS_ERR_NO_LOCK;
	graph_lock_pid = getpid();
	return 0;
}

uint32_t release_graphics_lock() {
	if (!has_graphics_lock(getpid()))
		return GRAPHICS_ERR_NO_LOCK;
	graph_lock_pid = 0;
	return 0;
}

extern void vesa32();
uint8_t error;
uint16_t framebuffer_width;
uint16_t framebuffer_height;
uint32_t framebuffer_pitch;
uint8_t framebuffer_bpp;
void *framebuffer_paddr = NULL;
void *framebuffer_addr = NULL;
uint32_t framebuffer_pages;

graphics_mode set_resolution(uint32_t width, uint32_t height, uint8_t bits) {
	if (!has_graphics_lock(getpid()))
		return (graphics_mode){.bpp = GRAPHICS_ERR_NO_LOCK};

	if (bits != 16 && bits != 24 && bits != 32) // Disallow 15 bit mode so the calculations work
		return (graphics_mode){.bpp = GRAPHICS_ERR_INVALID_BPP};

	dprintf("[VESA] Initiating VESA graphics change to %ux%u@%u bpp\n", width, height, bits);

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
	init_kbd();
	if (error)
		return (graphics_mode){.bpp = error};

	if (framebuffer_addr) {
		iofree_pages(framebuffer_addr, framebuffer_pages);
	}

	framebuffer_pitch = framebuffer_width * framebuffer_bpp / 8;
	framebuffer_pages = ((framebuffer_height * framebuffer_pitch) + PAGE_SIZE - 1) / PAGE_SIZE;
	framebuffer_addr = ioalloc_pages(framebuffer_paddr, framebuffer_pages);
	
	if (!framebuffer_addr)
		return (graphics_mode){.bpp = GRAPHICS_ERR_MODE_SET_FAIL};
	
	graphics_mode retval = {0};
	retval.bpp = bits;
	retval.height = height;
	retval.width = width;

	dprintf("[VESA] Graphics mode set: %ux%u@%u bpp\n", framebuffer_width, framebuffer_height, framebuffer_bpp);

	return retval;
}

void fastmemcpy32(char *dst, char *src, size_t size) {
	for (size_t i = 0; i < size; i++) {
		dst[i] = src[i];
	}
}

uint32_t copy_framebuffer(void *addr) {
	if (!has_graphics_lock(getpid()))
		return GRAPHICS_ERR_NO_LOCK;

	if (!framebuffer_addr)
		return GRAPHICS_ERR_NO_FRAMEBUFFER;

	if (!addr)
		return GRAPHICS_ERR_NO_FRAMEBUFFER;

	fastmemcpy32(framebuffer_addr, addr, framebuffer_width * framebuffer_height * (framebuffer_bpp / 8));
	return 0;
}

uint32_t get_framebuffer_size() {
	return framebuffer_pitch * framebuffer_height;
}

uint32_t set_text_mode() {
	if (!has_graphics_lock(getpid()))
		return GRAPHICS_ERR_NO_LOCK;

	dprintf("[VESA] Returning to VGA text mode\n");

	identity_map_pages((paddr_t)0x7000, 2);
	asm("\
		pusha;\
		mov $2,%%ch;\
		call vesa32;\
		mov %%al,error;\
		popa;\
	":::"eax","ebx","ecx");
	identity_free_pages((void *)0x7000, 2);
	init_pit(1000);
	init_kbd();

	if (error)
		return error;

	terminal_clear();

	if (framebuffer_addr) {
		iofree_pages(framebuffer_addr, framebuffer_pages);
	}

	framebuffer_width = 0;
	framebuffer_height = 0;
	framebuffer_pitch = 0;
	framebuffer_bpp = 0;
	framebuffer_paddr = NULL;
	framebuffer_addr = NULL;
	framebuffer_pages = 0;

	return 0;
}
