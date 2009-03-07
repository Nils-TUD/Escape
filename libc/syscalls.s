[BITS 32]

; the syscall-numbers
SYSCALL_PID					equ 0
SYSCALL_PPID				equ 1
SYSCALL_DEBUGCHAR		equ 2
SYSCALL_FORK				equ 3
SYSCALL_EXIT				equ	4
SYSCALL_OPEN				equ 5
SYSCALL_CLOSE				equ 6
SYSCALL_READ				equ 7
SYSCALL_REG					equ 8
SYSCALL_UNREG				equ 9
SYSCALL_CHGSIZE			equ 10
SYSCALL_MAPPHYS			equ	11
SYSCALL_WRITE				equ	12
SYSCALL_YIELD				equ 13
SYSCALL_SWAIT				equ 14
SYSCALL_REQIOPORTS	equ 15
SYSCALL_RELIOPORTS	equ 16
SYSCALL_DUPFD				equ	17
SYSCALL_REDIRFD			equ 18
SYSCALL_ADDINTRPTL	equ 19
SYSCALL_REMINTRPTL	equ 20

; the IRQ for syscalls
SYSCALL_IRQ					equ	0x30

; macros for the different syscall-types (void / ret-value, arg-count, error-handling)

%macro SYSC_VOID_0ARGS 2
[global %1]
%1:
	push	ebp
	mov		ebp,esp
	push	DWORD %2							; push syscall-number
	int		SYSCALL_IRQ
	add		esp,4									; remove from stack
	leave
	ret
%endmacro

%macro SYSC_VOID_1ARGS 2
[global %1]
%1:
	push	ebp
	mov		ebp,esp
	mov		eax,[ebp + 8]
	push	DWORD eax							; push arg1
	push	DWORD %2							; push syscall-number
	int		SYSCALL_IRQ
	add		esp,8									; remove from stack
	leave
	ret
%endmacro

%macro SYSC_RET_0ARGS 2
[global %1]
%1:
	push	ebp
	mov		ebp,esp
	push	DWORD 0								; needed for ret-value
	push	DWORD %2							; push syscall-number
	int		SYSCALL_IRQ
	add		esp,4									; remove from stack
	pop		eax										; return value
	leave
	ret
%endmacro

%macro SYSC_RET_1ARGS_ERR 2
[global %1]
%1:
	push	ebp
	mov		ebp,esp
	mov		eax,[ebp + 8]					; push arg1
	push	eax
	push	DWORD %2							; push syscall-number
	int		SYSCALL_IRQ
	pop		eax										; pop error-code
	test	eax,eax
	jz		%1NoError							; no-error?
	mov		[lastError],eax				; store error-code
	add		esp,4
	jmp		%1Ret
%1NoError:
	pop		eax
%1Ret:
	leave
	ret
%endmacro

%macro SYSC_RET_2ARGS_ERR 2
[global %1]
%1:
	push	ebp
	mov		ebp,esp
	mov		eax,[ebp + 12]				; push arg2
	push	eax
	mov		eax,[ebp + 8]					; push arg1
	push	eax
	push	DWORD %2							; push syscall-number
	int		SYSCALL_IRQ
	pop		eax										; pop error-code
	test	eax,eax
	jz		%1NoError							; no-error?
	mov		[lastError],eax				; store error-code
	add		esp,8
	jmp		%1Ret
%1NoError:
	pop		eax
	add		esp,4
%1Ret:
	leave
	ret
%endmacro

%macro SYSC_RET_3ARGS_ERR 2
[global %1]
%1:
	push	ebp
	mov		ebp,esp
	mov		eax,[ebp + 16]				; push arg3
	push	eax
	mov		eax,[ebp + 12]				; push arg2
	push	eax
	mov		eax,[ebp + 8]					; push arg1
	push	eax
	push	DWORD %2							; push syscall-number
	int		SYSCALL_IRQ
	pop		eax										; pop error-code
	test	eax,eax
	jz		%1NoError							; no-error?
	mov		[lastError],eax				; store error-code
	add		esp,12
	jmp		%1Ret
%1NoError:
	pop		eax
	add		esp,8
%1Ret:
	leave
	ret
%endmacro

%macro SYSC_RET_4ARGS_ERR 2
[global %1]
%1:
	push	ebp
	mov		ebp,esp
	mov		eax,[ebp + 20]				; push arg4
	push	eax
	mov		eax,[ebp + 16]				; push arg3
	push	eax
	mov		eax,[ebp + 12]				; push arg2
	push	eax
	mov		eax,[ebp + 8]					; push arg1
	push	eax
	push	DWORD %2							; push syscall-number
	int		SYSCALL_IRQ
	pop		eax										; pop error-code
	test	eax,eax
	jz		%1NoError							; no-error?
	mov		[lastError],eax				; store error-code
	add		esp,16
	jmp		%1Ret
%1NoError:
	pop		eax
	add		esp,12
%1Ret:
	leave
	ret
%endmacro
