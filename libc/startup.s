[BITS 32]

[global init]
[extern main]

ALIGN 4

init:
	call	main

	; we want to stay here...
	jmp		$
