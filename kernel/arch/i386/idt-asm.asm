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

extern temp_tss

extern new_temp_tss
extern ready_esp

extern syscall_temp_tss
extern run_syscall
extern yield_esp
 
global switch_task
global run_syscall_asm

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
  mov dword [yield_esp],esp
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
  
  cmp eax,1
  jne a  
  nop
  
a:
  call run_syscall
  
  mov esp,dword [yield_esp]
  
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
