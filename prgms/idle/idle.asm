bits 32
org 0x08048000
file_start:
elf_hdr:
	db 0x7F, 'E', 'L', 'F'
	db 1
	db 1
	db 1
	db 0
	dd 0
	dd 0
	dw 2
	dw 3
	dd 1
	dd _start
	dd ph_hdr - 0x08048000
	dd 0
	dd 0
	dw elf_hdr_end-elf_hdr
	dw ph_hdr_end-ph_hdr
	dw 1
	dw 0
	dw 0
	dw 0
elf_hdr_end:
ph_hdr:
	dd 1
	dd 0
	dd 0x08048000
	dd 0x08048000
	dd file_end-file_start
	dd file_end-file_start
	dd 5
	dd 0x00001000
ph_hdr_end:

_start:
	jmp $
file_end:
