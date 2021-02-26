global irq0
global irq1
global irq2
global irq3
global irq4
global irq5
global irq6
global irq7
global irq8
global irq9
global irq10
global irq11
global irq12
global irq13
global irq14
global irq15

global default_handler
 
global load_idt
 
;global irq0_handler
;global irq1_handler
;global irq2_handler
;global irq3_handler
;global irq4_handler
;global irq5_handler
;global irq6_handler
;global irq7_handler
;global irq8_handler
;global irq9_handler
;global irq10_handler
;global irq11_handler
;global irq12_handler
;global irq13_handler
;global irq14_handler
;global irq15_handler
 
extern irq0_handler
extern irq1_handler
extern irq2_handler
extern irq3_handler
extern irq4_handler
extern irq5_handler
extern irq6_handler
extern irq7_handler
extern irq8_handler
extern irq9_handler
extern irq10_handler
extern irq11_handler
extern irq12_handler
extern irq13_handler
extern irq14_handler
extern irq15_handler

extern unhandled_interrupt
extern exception_page_fault

extern temp_tss

extern new_temp_tss
extern ready_esp

extern syscall_temp_tss
extern run_syscall
 
global switch_task
global run_syscall_asm

orig_eax dd 0
retaddr dd 0
errcode dd 0

global page_fault

page_fault:
  mov dword [orig_eax],eax
  pop eax
  mov dword [errcode],eax
  mov [ready_esp],esp
  pop eax
  mov dword [retaddr],eax
  push eax
  mov eax, dword [orig_eax]
  pusha
  mov eax, dword [errcode]
  push eax
  mov eax, dword [retaddr]
  push eax
  call exception_page_fault
  popa
  iret

switch_task:
  mov esp,dword [ready_esp]
  
  add esp,0xC
  pop eax
  mov eax,dword [new_temp_tss+56] ;esp
  push eax
  sub esp,0xC
  
  pop eax
  mov eax,dword [new_temp_tss+32] ;eip
  push eax
  
  mov eax,dword [new_temp_tss+36] ;eflags
  push eax
  popf
  
  mov eax,dword [new_temp_tss+40]
  mov ebx,dword [new_temp_tss+52]
  mov ecx,dword [new_temp_tss+44]
  mov edx,dword [new_temp_tss+48]
  mov edi,dword [new_temp_tss+68]
  mov esi,dword [new_temp_tss+64]
  mov ebp,dword [new_temp_tss+60]
  
  iret
  
run_syscall_asm:
  mov dword [ready_esp],esp
  mov dword [syscall_temp_tss+40],eax
  mov dword [syscall_temp_tss+52],ebx
  mov dword [syscall_temp_tss+44],ecx
  mov dword [syscall_temp_tss+48],edx
  mov dword [syscall_temp_tss+68],edi
  mov dword [syscall_temp_tss+64],esi
  mov dword [syscall_temp_tss+60],ebp
  pushf
  pop eax
  mov dword [syscall_temp_tss+36],eax ;eflags
  pop eax
  push eax
  mov dword [syscall_temp_tss+32],eax ;eip
  add esp,0xC
  pop eax
  push eax
  sub esp,0xC
  mov dword [syscall_temp_tss+56],eax ;esp
  mov eax, dword [syscall_temp_tss+40]
  
  call run_syscall
  
  mov esp,dword [ready_esp]
  
  add esp,0xC
  pop ebx
  mov ebx,dword [syscall_temp_tss+56] ;esp
  push ebx
  sub esp,0xC
  
  pop ebx
  mov ebx,dword [syscall_temp_tss+32] ;eip
  push ebx
  
  mov ebx,dword [syscall_temp_tss+36] ;eflags
  push ebx
  popf
  
  mov ebx,dword [syscall_temp_tss+52]
  mov ecx,dword [syscall_temp_tss+44]
  mov edx,dword [syscall_temp_tss+48]
  mov edi,dword [syscall_temp_tss+68]
  mov esi,dword [syscall_temp_tss+64]
  mov ebp,dword [syscall_temp_tss+60]
  
  iret
  
irq0:
  mov dword [ready_esp],esp
  mov dword [temp_tss+40],eax
  mov dword [temp_tss+52],ebx
  mov dword [temp_tss+44],ecx
  mov dword [temp_tss+48],edx
  mov dword [temp_tss+68],edi
  mov dword [temp_tss+64],esi
  mov dword [temp_tss+60],ebp
  pushf
  pop eax
  mov dword [temp_tss+36],eax ;eflags
  pop eax
  push eax
  mov dword [temp_tss+32],eax ;eip
  add esp,0xC
  pop eax
  push eax
  sub esp,0xC
  mov dword [temp_tss+56],eax ;esp
  mov eax, dword [temp_tss+40]
  pusha
  call irq0_handler
  popa
  iret
 
irq1:
  pusha
  call irq1_handler
  popa
  iret
 
irq2:
  pusha
  call irq2_handler
  popa
  iret
 
irq3:
  pusha
  call irq3_handler
  popa
  iret
 
irq4:
  pusha
  call irq4_handler
  popa
  iret
 
irq5:
  pusha
  call irq5_handler
  popa
  iret
 
irq6:
  pusha
  call irq6_handler
  popa
  iret
 
irq7:
  pusha
  call irq7_handler
  popa
  iret
 
irq8:
  pusha
  call irq8_handler
  popa
  iret
 
irq9:
  pusha
  call irq9_handler
  popa
  iret
 
irq10:
  pusha
  call irq10_handler
  popa
  iret
 
irq11:
  pusha
  call irq11_handler
  popa
  iret
 
irq12:
  pusha
  call irq12_handler
  popa
  iret
 
irq13:
  pusha
  call irq13_handler
  popa
  iret
 
irq14:
  pusha
  call irq14_handler
  popa
  iret
 
irq15:
  pusha
  call irq15_handler
  popa
  iret
 
default_handler:
  pusha
  call unhandled_interrupt
  popa
  iret

load_idt:
	mov edx, [esp + 4]
	lidt [edx]
	sti
	ret
	
	global test_int
test_int:
	xor eax, eax
	int 0x80
	ret
	
	extern last_stack
	extern last_entrypoint
	
	; Load all the segment registers with the usermode data selector
	; Then push the stack segment and the stack pointer (we need to change this)
	; Then modify the flags so they enable interrupts on iret
	; Push the code selector on the stack
	; Push the entrypoint of the program in memory, then iret to enter usermode
	global enter_usermode
enter_usermode:
	cli
	mov ax,0x23
	mov ax, ds
	mov ax, es
	mov ax, fs
	mov ax, gs
	push 0x23
	mov eax,[last_stack]
	push eax
	pushf
	pop eax
	or eax,0x200
	push eax
	push 0x1B
	mov eax,[last_entrypoint]
	push eax
	mov eax,0
	mov ebx,0
	mov ecx,0xBFFFE000
	mov edx,0xBFFFF000
	mov esi,0
	mov edi,0
	mov ebp,0
	iret
	
	; Exit usermode
	global exit_usermode
exit_usermode:
	cli
	mov ax,0x10
	mov ax, ds
	mov ax, es
	mov ax, fs
	mov ax, gs
	push 0x10
	mov eax,[last_stack]
	push eax
	pushf
	pop eax
	mov eax,0x200
	push eax
	push 0x8
	mov eax,[last_entrypoint]
	push eax
	iret
	
; The following was modified from Omarrx024's VESA tutorial on the OSDev Wiki
; (https://wiki.osdev.org/User:Omarrx024/VESA_Tutorial)
; This code is used under CC0 1.0 (see https://wiki.osdev.org/OSDev_Wiki:Copyrights for details)

; VESA32 function:
; Switch to realmode, change resolution, and return to protected mode.
; 
; Inputs:
;	ch - Function (0=change resolution, 1=Get resolution of available mode #ax)
;	mode 0:
;		ax - Resolution width
;		bx - Resolution height
;		cl - Bit depth (bpp)
;	mode 1:
;		ax - mode number
; 
; Outputs:
;	mode 0:
;		al - Error code
;			0 = success
;			1 = mode not found
;			2 = BIOS error
;		ebx - Framebuffer address (physical)
;	mode 1:
;		ax - Width
;		bx - Height
;		cl - BPP
;
; Note: you will most likely have to reload the PIT

; Macros to make it easier to access data and code copied to 0x7C00
%define INT32_LENGTH (_int32_end-_int32)
%define FIXADDR(addr) (((addr)-_int32)+0x7C00)

; Macros for the storevars at 0x7C00
%define sv_width FIXADDR(storevars.width)
%define sv_height FIXADDR(storevars.height)
%define sv_bpp FIXADDR(storevars.bpp)
%define sv_func FIXADDR(storevars.func)
%define sv_segment FIXADDR(storevars.segment)
%define sv_offset FIXADDR(storevars.offset)
%define sv_mode FIXADDR(storevars.mode)

global vesa32
vesa32:
	cli
	
	; Store registers so we can use them
	mov [storevars.width],ax
	mov [storevars.height],bx
	mov [storevars.bpp],cl
	mov [storevars.func],ch
	
	; Copy the _int32 function (et al) to 0x7C00 (below 1MiB)
	mov esi,_int32
	mov edi,0x7C00
	mov ecx,INT32_LENGTH
	cld
	rep movsb
	
	; Relocate the stored variables to where the rest of the data is
	mov ax,[storevars.width]
	mov [sv_width],ax
	mov ax,[storevars.height]
	mov [sv_height],ax
	mov al,[storevars.bpp]
	mov [sv_bpp],al
	
	; Jump to code under 1MiB so we can run in 16 bit mode
	jmp 0x00007C00
[BITS 32]
_int32:
	; Store any remaining registers so we don't mess anything up in C
	mov [store32.edx],edx
	mov [store32.esi],esi
	mov [store32.edi],edi
	mov [store32.esp],esp
	mov [store32.ebp],ebp
	
	; Store the cr3 in case the BIOS messes it up
	mov eax,cr3
	mov [store32.cr3],eax
	
	; Disable paging
	mov eax,cr0
	and eax,0x7FFFFFFF
	mov cr0,eax
	
	; Store existing GDTs and IDTs and load temporary ones
	sgdt [FIXADDR(gdt32)]
	sidt [FIXADDR(idt32)]
	lgdt [FIXADDR(gdt16)]
	lidt [FIXADDR(idt16)]
	
	; Switch to 16 bit protected mode
	jmp word 0x08:FIXADDR(_intp16)
[BITS 16]
_intp16:
	; Load all the segment registers with the data segment
	mov ax,0x10
	mov ds,ax
	mov es,ax
	mov fs,ax
	mov gs,ax
	mov ss,ax
	
	; Disable protected mode
	mov eax,cr0
	and al, 0xFE
	mov cr0,eax
	
	; Jump to realmode
	jmp word 0x0000:FIXADDR(_intr16)
_intr16:
	; Load all the data segments with 0
	mov ax,0
	mov ss,ax
	mov ds,ax
	mov es,ax
	mov fs,ax
	mov gs,ax
	; Set a temporary stack
	mov sp,0x7B00
	
	
	; Get a list of modes
	push es
	mov ax,0x4F00
	mov di,FIXADDR(vbe_info)
	int 0x10
	pop es
	
	; Check for error
	cmp ax, 0x4F
	jne .error

	; Set up the registers with the segment:offset of the modes
	mov ax, word[FIXADDR(vbe_info.vmodeoff)]
	mov [sv_offset],ax
	mov ax, word[FIXADDR(vbe_info.vmodeseg)]
	mov [sv_segment],ax
	
	mov ax, [sv_segment]
	mov fs,ax
	mov si, [sv_offset]
	
	mov al,[sv_func]
	cmp al,1
	je .getmode
	cmp al,0
	jne .error2

.find_mode:
	; Increment the mode
	mov dx,[fs:si]
	add si,2
	mov [sv_offset],si
	mov [sv_mode], dx
	mov ax,0
	mov fs,ax
	
	; Make sure we haven't run out of modes
	cmp word[sv_mode],0xFFFF
	je .error2
	
	; List the values for the selected mode
	push es
	mov ax,0x4f01
	mov cx,[sv_mode]
	mov di, FIXADDR(vbe_screen)
	int 0x10
	pop es
	
	; Check for error
	cmp ax, 0x4F
	jne .error
	
	; Check width
	mov ax, [sv_width]
	cmp ax, [FIXADDR(vbe_screen.width)]
	jne .next_mode
	
	; Check height
	mov ax, [sv_height]
	cmp ax, [FIXADDR(vbe_screen.height)]
	jne .next_mode
	
	; Check bpp
	mov al, [sv_bpp]
	cmp al, [FIXADDR(vbe_screen.bpp)]
	jne .next_mode

	; We've found our mode. Now switch to it
	push es
	mov ax, 0x4F02
	mov bx, [sv_mode]
	or bx, 0x4000
	mov di,0
	int 0x10
	pop es
	
	; Check for any errors
	cmp ax, 0x4F
	jne .error

	; Set up return values
	mov ax,0
	mov ebx,[FIXADDR(vbe_screen.buffer)]
	; Start the transition back to protected mode
	jmp .returnpm

; Any BIOS errors use this function
.error:
	mov ax,2
	jmp .returnpm
	
; This error is only for if the requested mode could not be found
.error2:
	mov ax,1
	jmp .returnpm

; Get the address for the next mode
.next_mode:
	mov ax, [sv_segment]
	mov fs,ax
	mov si, [sv_offset]
	jmp .find_mode

; Get the values for mode stored in ax at start
.getmode:
	mov ax, [sv_width]
	add ax,ax
	add si,ax
	mov dx, [fs:si]
	mov [sv_mode],dx
	mov ax,0
	mov fs,ax
	
	cmp word [sv_mode],0xFFFF
	je .error2
	
	push es
	mov ax,0x4f01
	mov cx,[sv_mode]
	mov di, FIXADDR(vbe_screen)
	int 0x10
	pop es
	
	cmp ax,0x4F
	jne .error
	
	mov ax,[FIXADDR(vbe_screen.width)]
	mov [FIXADDR(storevars.width)],ax
	
	mov ax,[FIXADDR(vbe_screen.height)]
	mov [FIXADDR(storevars.height)],ax
	
	mov al,[FIXADDR(vbe_screen.bpp)]
	mov [FIXADDR(storevars.bpp)],al
	
	mov ax,0
	
; Return to protected mode!
.returnpm:
	; Store the return values
	mov [FIXADDR(storevars.error)],ax
	mov [FIXADDR(storevars.buffer)],ebx
	
	; Turn on protected mode (this is same as "or cr0,1")
	mov eax,cr0
	inc eax
	mov cr0,eax
	
	; Load 32 bit GDT
	lgdt [FIXADDR(gdt32)]
	; Jump to 32 bit protected mode
	jmp 0x08:FIXADDR(returnpm32)
	
[BITS 32]
; We're back in 32 bit protected mode land!
returnpm32:
	; Load all the data segments
	mov ax,0x10
	mov ss,ax
	mov ds,ax
	mov es,ax
	mov fs,ax
	mov gs,ax
	; Use the protected mode IDT
	lidt [FIXADDR(idt32)]
	
	; Re-enable paging
	mov eax,cr0
	or eax,0x80000000
	mov cr0,eax
	; Restore cr3
	mov eax,[store32.cr3]
	mov cr3,eax
	
	; Reload GDT with paging enabled (otherwise we will triple fault on sti)
	lgdt [FIXADDR(gdt32)]
	
	; Restore PIC (see idt.c for values)
	mov al,0x11
	out 0x20,al
	out 0xA0,al
	mov al,0x20
	out 0x21,al
	mov al,40
	out 0xA1,al
	mov al,4
	out 0x21,al
	sub al,2
	out 0xA1,al
	dec al
	out 0x21,al
	out 0xA1,al
	xor al,al
	out 0x21,al
	out 0xA1,al
	
	mov al,[sv_func]
	cmp al,1
	je .mode1
	
	mov eax,[FIXADDR(storevars.error)]
	mov ebx,[FIXADDR(storevars.buffer)]
	jmp .restore

.mode1:
	mov ax,[FIXADDR(storevars.width)]
	mov bx,[FIXADDR(storevars.height)]
	mov cl,[FIXADDR(storevars.bpp)]
	mov ch,[FIXADDR(storevars.error)]
	
.restore:
	; Restore all registers except output registers
	mov edx,[store32.edx]
	mov esi,[store32.esi]
	mov edi,[store32.edi]
	mov esp,[store32.esp]
	mov ebp,[store32.ebp]
	
	; Re-enable interrupts
	sti
	; Finally, return to the callee
	ret

gdt32:
	dw 0
	dd 0
	
idt32:
	dw 0
	dd 0
	
idt16:
	dw 0x03FF
	dd 0
	
gdt16_struct:
	dq 0
	
	dw 0xFFFF
	dw 0
	db 0
	db 10011010b
	db 10001111b
	db 0
	
	dw 0xFFFF
	dw 0
	db 0
	db 10010010b
	db 10001111b
	db 0
	
gdt16:
	dw gdt16 - gdt16_struct - 1
	dd FIXADDR(gdt16_struct)

storevars:
	.width 		dw 0
	.height 	dw 0
	.bpp 		db 0
	.func 		db 0
	.segment	dw 0
	.offset 	dw 0
	.mode		dw 0
	.buffer		dd 0
	.error		db 0

vbe_screen:
	.attr		dw 0
	.unused0	db 0
	.unused1	db 0
	.unused2	dw 0
	.unused3	dw 0
	.unused4	dw 0
	.unused5	dw 0
	.unused7	dd 0
	.pitch		dw 0
	.width		dw 0
	.height		dw 0
	.unused8	db 0
	.unused9	db 0
	.unusedA	db 0
	.bpp		db 0
	.unusedB	db 0
	.unusedC	db 0
	.unusedD	db 0
	.unusedE	db 0
	.reserved0	db 0
	
	.redmask	db 0
	.redpos		db 0
	.greenmask	db 0
	.greenpos	db 0
	.bluemask	db 0
	.bluepos	db 0
	.rmask		db 0
	.rpos		db 0
	.cattrs		db 0
	
	.buffer		dd 0
	.sm_off		dd 0
	.sm_size	dw 0
	.table times 206 db 0
	
vbe_info:
	.signature	db "VBE2"
	.version	dw 0
	.oem		dd 0
	.cap		dd 0
	.vmodeoff	dw 0
	.vmodeseg	dw 0
	.vmem		dw 0
	.softrev	dw 0
	.vendor		dd 0
	.pname		dd 0
	.prev		dd 0
	.reserved 	times 222 db 0
	.oemdata	times 256 db 0
	
_int32_end:

store32:
	.ecx dd 0
	.edx dd 0
	.esi dd 0
	.edi dd 0
	.esp dd 0
	.ebp dd 0
	.cr3 dd 0