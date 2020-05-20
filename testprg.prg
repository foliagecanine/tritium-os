org 0x100000
bits 32
mov ebx,mytext
xor eax, eax
int 0x80
jmp $

mytext db "Hello from program world!",0
