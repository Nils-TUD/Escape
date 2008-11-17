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
