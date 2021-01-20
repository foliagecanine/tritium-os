org 0x100000
bits 32
mov eax,25
int 0x80
mov ebx,hello
xor eax, eax
int 0x80
mov eax,2
int 0x80
jmp $

hello db "Hello world!",10,0
