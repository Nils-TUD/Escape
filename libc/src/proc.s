[BITS 32]

[global getpid]
[global getppid]

SYSCALL_PIDNO		equ 0
SYSCALL_PPIDNO	equ 1
SYSCALL_IRQ			equ	0x30

; u32 getpid(void);
getpid:
	push	ebp
	mov		ebp,esp
	push	DWORD 0								; needed for ret-value
	push	DWORD SYSCALL_PIDNO		; push syscall-number
	int		SYSCALL_IRQ
	add		esp,4									; remove from stack
	pop		eax										; return pid
	leave
	ret

; u32 getppid(void);
getppid:
	push	ebp
	mov		ebp,esp
	push	DWORD 0								; needed for ret-value
	push	DWORD SYSCALL_PPIDNO	; push syscall-number
	int		SYSCALL_IRQ
	add		esp,4									; remove from stack
	pop		eax										; return ppid
	leave
	ret
