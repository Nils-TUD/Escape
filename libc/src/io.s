[BITS 32]

[extern lastError]
[global open]
[global close]

SYSCALL_OPEN	equ 5
SYSCALL_CLOSE	equ 6
SYSCALL_IRQ		equ	0x30

; s32 open(cstring path,u8 mode);
open:
	push	ebp
	mov		ebp,esp
	mov		eax,[ebp + 12]				; push mode
	push	eax
	mov		eax,[ebp + 8]					; push path
	push	eax
	push	DWORD SYSCALL_OPEN		; push syscall-number
	int		SYSCALL_IRQ
	pop		eax										; pop error-code
	test	eax,eax
	jz		openNoError						; no-error?
	mov		[lastError],eax				; store error-code
	add		esp,8
	jmp		openRet
openNoError:
	pop		eax
	add		esp,4
openRet:
	leave
	ret

; void close(s32 fd);
close:
	push	ebp
	mov		ebp,esp
	mov		eax,[ebp + 8]					; push fd
	push	eax
	push	DWORD SYSCALL_CLOSE		; push syscall-number
	int		SYSCALL_IRQ
	add		esp,4									; remove from stack
	leave
	ret

