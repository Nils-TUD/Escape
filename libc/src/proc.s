[BITS 32]

[global getpid]
[global getppid]
[global fork]
[global exit]

SYSCALL_PID		equ 0
SYSCALL_PPID	equ 1
SYSCALL_FORK	equ 3
SYSCALL_EXIT	equ	4
SYSCALL_IRQ		equ	0x30

; tPid getpid(void);
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

; tPid getppid(void);
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

; s32 fork(void);
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

; void exit(u32 errorCode);
exit:
	push	ebp
	mov		ebp,esp
	mov		eax,[ebp + 8]
	push	DWORD eax							; push error-code
	push	DWORD SYSCALL_EXIT		; push syscall-number
	int		SYSCALL_IRQ
	add		esp,8									; remove from stack
	leave
	ret
