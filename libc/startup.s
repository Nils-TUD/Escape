[BITS 32]

[global init]
[extern main]
[extern exit]

ALIGN 4

init:
	call	main

	; call exit with return-value of main
	push	eax
	call	exit

	; just to be sure
	jmp		$

;[extern sigTest]
; restores registers and stack after a signal-handling
;sigRetFunc:
;	add		esp,4
;	pop		esi
;	pop		edi
;	pop		edx
;	pop		ecx
;	pop		ebx
;	pop		eax
;	cmp		DWORD [esp-6*4],0
;	je		sigFinished
;	mov		eax,[esp]
;	push  eax
;	call	sigTest
;	add		esp,4
;sigFinished:
;	ret
