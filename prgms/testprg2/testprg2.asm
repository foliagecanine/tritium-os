org 0x100000
bits 32
mov ebx,hello
xor eax, eax
int 0x80
push mynumber
pop ebx
xor eax, eax
int 0x80
mov eax,2
int 0x80
jmp $

hello db "Hello from program world!",10,0
mynumber db "My name is TESTPRG2.PRG.",10,0
