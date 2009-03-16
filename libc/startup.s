[BITS 32]

[global init]
[extern main]
[extern exit]
[extern ackSignal]

ALIGN 4

init:
	call	main

	; call exit with return-value of main
	push	eax
	call	exit

	; just to be sure
	jmp		$


; all signal-handler return to this "function" (address 0xd)
sigRetFunc:
	; ack signal so that the kernel knows that we accept another signal
	push	DWORD [esp - 7 * 4]
	call	ackSignal
	; remove arg of ackSignal and the signal-handler
	add		esp,8
	; restore register
	pop		esi
	pop		edi
	pop		edx
	pop		ecx
	pop		ebx
	pop		eax
	; return to the instruction before the signal
	ret
