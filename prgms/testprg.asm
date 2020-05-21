org 0x100000
bits 32
mov ebx,hello
xor eax, eax
int 0x80
push ebx
jmp $

hello db "Hello from program world!",10
      db "Just a heads up, this will probably cause a page fault.",10,0
