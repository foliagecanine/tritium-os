; Based off of https://wiki.osdev.org/Setting_Up_Paging

	GLOBAL load_pagedir
	
load_pagedir:
	push ebp
	mov ebp, esp
	mov eax, [esp+8]
	mov cr3, eax
	mov esp,ebp
	pop ebp
	ret
	
	GLOBAL enablePaging

enablePaging:
	push ebp
	mov ebp,esp
	mov eax,cr0 ; Get value of cr0
	or eax, 0x800000000 ; Set paging bit
	mov cr0,eax
	mov esp,ebp
	pop ebp
	ret