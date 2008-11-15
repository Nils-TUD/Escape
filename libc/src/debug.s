[BITS 32]

[global debugChar]

SYSCALL_NO	equ 2
SYSCALL_IRQ	equ	0x30

ALIGN 4

; void debugChar(s8 c);
debugChar:
	push	ebp
	mov		ebp,esp
	mov		eax,[esp + 8]
	push	eax									; push char
	push	DWORD SYSCALL_NO		; push syscall-number
	int		SYSCALL_IRQ
	add		esp,8								; remove from stack
	leave
	ret
