bits 32
global _start
_start:
	mov ebx,hello
	xor eax, eax
	int 0x80
	push mynumber
	pop ebx
	xor eax, eax
	int 0x80
	jmp $

	hello db "Hello from program world!",10,0
	mynumber db "My name is TESTELF.PRG.",10,0
