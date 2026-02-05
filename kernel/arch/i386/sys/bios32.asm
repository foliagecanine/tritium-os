; BIOS32 function:
; Execute function in BIOS (see VESA32 in idt-asm.asm for VESA related bios functions)
; 
; Inputs:
;	al - Function
;	other registers, vary
; 
; Outputs:
;	varies
;
; Note: you will most likely have to reload the PIT

; Macros to make it easier to access data and code copied to 0x7C00
%define INT32_LENGTH (_int32_end-_int32)
%define FIXADDR(addr) (((addr)-_int32)+0x7C00)

; Macros for the storevars at 0x7C00
%define sv_bx FIXADDR(storevars.bx)
%define sv_cx FIXADDR(storevars.cx)
%define sv_dx FIXADDR(storevars.dx)
%define sv_si FIXADDR(storevars.si)
%define sv_di FIXADDR(storevars.di)
%define sv_func FIXADDR(storevars.func)

global bios32
bios32:
	cli
	
	; Store registers so we can use them
	mov [storevars.func],ax
	mov [storevars.bx],bx
	mov [storevars.cx],cx
	mov [storevars.dx],dx
	mov [storevars.si],si
	mov [storevars.di],di
	
	; Copy the _int32 function (et al) to 0x7C00 (below 1MiB)
	mov esi,_int32
	mov edi,0x7C00
	mov ecx,INT32_LENGTH
	cld
	rep movsb
	
	; Relocate the stored variables to where the rest of the data is
	mov ax,[storevars.bx]
	mov [sv_bx],ax
	mov ax,[storevars.cx]
	mov [sv_cx],ax
	mov ax,[storevars.dx]
	mov [sv_dx],ax
	mov ax,[storevars.si]
	mov [sv_si],ax
	mov ax,[storevars.di]
	mov [sv_di],ax
	mov ax,[storevars.func]
	mov [sv_func],ax
	
	; Jump to code under 1MiB so we can run in 16 bit mode
	jmp 0x00007C00
[BITS 32]
_int32:
	; Store any remaining registers so we don't mess anything up in C
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
	
	mov ax,[sv_func]
	cmp ax,0x1553
	je .bios_shutdown
	jmp .fn_err
	
.bios_shutdown:
	; Connect to realmode BIOS APM interface (if not already)
	mov ax,0x5301
	xor bx,bx
	mov dx,1
	int 0x15
	
	; Set APM to version 1.2 (if not already)
	mov ax,0x530E
	xor bx,bx
	mov cx,0x102
	int 0x15
	
	; Enable BIOS APM (if not already)
	mov ax, 0x5308
	mov bx,1
	mov cx,1
	int 0x15

	; Use BIOS function 0x5307 to shut down the computer.
	;mov ax,0x1000
	;mov ax,ss
	;mov sp,0xf000
	mov ax,0x5307
	mov bx,1
	mov cx,3
	int 0x15
	
	; Just do a hlt. Interrupts are already disabled. Tell the user to power off their computer manually.
	hlt

.fn_err:
	mov ax,0xFFFF
	
; Return to protected mode!
.returnpm:
	; Store the return values
	mov [FIXADDR(storevars.error)],ax
	
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
	
	mov ax,[FIXADDR(storevars.error)]
	
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
	.bx 		dw 0
	.cx 		dw 0
	.dx			dw 0
	.si			dw 0
	.di			dw 0
	.func 		dw 0
	.error		dw 0
	

store32:
	.esp dd 0
	.ebp dd 0
	.cr3 dd 0
_int32_end:
