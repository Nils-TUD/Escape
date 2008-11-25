[BITS 32]

[extern lastError]
[global regService]
[global unregService]

SYSCALL_REG		equ 8
SYSCALL_UNREG	equ 9
SYSCALL_IRQ		equ	0x30

; s32 regService(cstring name);
regService:
	push	ebp
	mov		ebp,esp
	mov		eax,[ebp + 8]					; push name
	push	eax
	push	DWORD SYSCALL_REG			; push syscall-number
	int		SYSCALL_IRQ
	pop		eax										; pop error-code
	test	eax,eax
	jz		regServiceNoError			; no-error?
	mov		[lastError],eax				; store error-code
	add		esp,4
	jmp		regServiceRet
regServiceNoError:
	pop		eax
regServiceRet:
	leave
	ret

; void unregService(void);
unregService:
	push	ebp
	mov		ebp,esp
	push	DWORD SYSCALL_UNREG		; push syscall-number
	int		SYSCALL_IRQ
	add		esp,4
	leave
	ret
