[BITS 32]

[global init]

ALIGN 4

%include "../../libc/syscalls.s"

init:
	; load modules first
	push	DWORD SYSCALL_LOADMODS
	int		SYSCALL_IRQ
	add		esp,4

	; now replace with init
	mov		eax,argp								; push arguments
	push	eax
	mov		eax,progName						; push path
	push	eax
	push	DWORD SYSCALL_EXEC			; push syscall-number
	int		SYSCALL_IRQ

	; we should not reach this
	jmp		$

argp:
	dd		progName
	dd		0

progName:
	db		"file:/apps/init"
	db		0
