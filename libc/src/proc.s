[BITS 32]

[global getpid]
[global getppid]
[global fork]

SYSCALL_PID		equ 0
SYSCALL_PPID	equ 1
SYSCALL_FORK	equ 3
SYSCALL_IRQ		equ	0x30

; u32 getpid(void);
getpid:
	push	ebp
	mov		ebp,esp
	push	DWORD 0								; needed for ret-value
	push	DWORD SYSCALL_PID			; push syscall-number
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
	push	DWORD SYSCALL_PPID		; push syscall-number
	int		SYSCALL_IRQ
	add		esp,4									; remove from stack
	pop		eax										; return ppid
	leave
	ret

; u16 fork(void);
fork:
	push	ebp
	mov		ebp,esp
	push	DWORD 0								; needed for ret-value
	push	DWORD SYSCALL_FORK		; push syscall-number
	int		SYSCALL_IRQ
	add		esp,4									; remove from stack
	pop		eax										; return pid
	leave
	ret
