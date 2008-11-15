[BITS 32]

[global debugChar]

ALIGN 4

; void debugChar(s8 c);
debugChar:
	push	ebp
	mov		ebp,esp
	mov		al,[esp+8]					; load value
	mov		dx,0xe9							; load port
	out		dx,al								; write to port
	;mov		dx,0x3f8						; load port
	;out		dx,al								; write to port
debugCharWait:
	;mov		dx,0x3fd						; load port
	;in		al,dx								; read from port
	;and		al,0x20
	;cmp		al,0
	;je		debugCharWait					; while(in(0x3fd) & 0x20) == 0);
	leave
	ret
