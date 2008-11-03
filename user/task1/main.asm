[BITS 32]

; exports
[global start]

; Multiboot constants
MULTIBOOT_PAGE_ALIGN	equ 1<<0
MULTIBOOT_MEMORY_INFO	equ 1<<1
MULTIBOOT_HEADER_MAGIC	equ 0x1BADB002
MULTIBOOT_HEADER_FLAGS	equ MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO
MULTIBOOT_CHECKSUM	equ -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

ALIGN 4
multiboot_header:
	dd		MULTIBOOT_HEADER_MAGIC
	dd		MULTIBOOT_HEADER_FLAGS
	dd		MULTIBOOT_CHECKSUM

; entry point
start:
	push	DWORD taskName
	call	puts
	add		esp,4
	jmp		start

; void puts(char *str);
puts:
	push	ebp
	mov		ebp,esp
	mov		eax,[esp+8]
	mov		ebx,0
putsLoop:
	mov		BYTE bl,[eax]				; load char
	cmp		bl,0								; reached \0 ?
	je		putsFinished
	push	eax
	push	ebx
	call	putchar
	add		esp,4
	pop		eax
	add		eax,1								; str++
	jmp		putsLoop
putsFinished:
	leave
	ret

; void putchar(char c);
putchar:
	push	ebp
	mov		ebp,esp
	mov		dx,0xe9							; load port
	mov		al,[esp+8]					; load value
	out		dx,al								; write to port
putcharWait:
	mov		dx,0x3fd						; load port
	in		al,dx								; read from port
	and		al,0x20
	cmp		al,0
	je		putcharWait					; while(in(0x3fd) & 0x20) == 0);
	leave
	ret

ALIGN 1

taskName:
	db		"Task1"
	db		0xA
	db		0
