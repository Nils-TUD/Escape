[BITS 32]

[global init]

ALIGN 4

%include "../../libc/syscalls.s"

init:
	; load modules first
	mov		eax,SYSCALL_LOADMODS
	int		SYSCALL_IRQ

	; now replace with init
	mov		ecx,progName						; set path
	mov		edx,argp								; set arguments
	mov		eax,SYSCALL_EXEC				; set syscall-number
	int		SYSCALL_IRQ

	; we should not reach this
	jmp		$

argp:
	dd		progName
	dd		0

progName:
	db		"file:/bin/init"
	db		0
