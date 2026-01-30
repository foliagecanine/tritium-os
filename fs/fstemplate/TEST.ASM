mov eax,0
mov ebx,mystr
int 0x80

mov eax,2
int 0x80

mystr:
.db 'H', 'i', 10, 0
