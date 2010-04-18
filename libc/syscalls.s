;
; $Id$
; Copyright (C) 2008 - 2009 Nils Asmussen
;
; This program is free software; you can redistribute it and/or
; modify it under the terms of the GNU General Public License
; as published by the Free Software Foundation; either version 2
; of the License, or (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program; if not, write to the Free Software
; Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
;

[BITS 32]

; the syscall-numbers
SYSCALL_PID						equ 0
SYSCALL_PPID					equ 1
SYSCALL_DEBUGCHAR			equ 2
SYSCALL_FORK					equ 3
SYSCALL_EXIT					equ	4
SYSCALL_OPEN					equ 5
SYSCALL_CLOSE					equ 6
SYSCALL_READ					equ 7
SYSCALL_REG						equ 8
SYSCALL_UNREG					equ 9
SYSCALL_CHGSIZE				equ 10
SYSCALL_MAPPHYS				equ	11
SYSCALL_WRITE					equ	12
SYSCALL_YIELD					equ 13
SYSCALL_REQIOPORTS		equ 14
SYSCALL_RELIOPORTS		equ 15
SYSCALL_DUPFD					equ	16
SYSCALL_REDIRFD				equ 17
SYSCALL_WAIT					equ 18
SYSCALL_SETSIGH				equ	19
SYSCALL_UNSETSIGH			equ	20
SYSCALL_ACKSIG				equ	21
SYSCALL_SENDSIG				equ 22
SYSCALL_EXEC					equ 23
SYSCALL_EOF						equ 24
SYSCALL_LOADMODS			equ	25
SYSCALL_SLEEP					equ 26
SYSCALL_SEEK					equ 27
SYSCALL_STAT					equ 28
SYSCALL_DEBUG					equ 29
SYSCALL_CREATESHMEM		equ 30
SYSCALL_JOINSHMEM			equ 31
SYSCALL_LEAVESHMEM		equ 32
SYSCALL_DESTROYSHMEM	equ 33
SYSCALL_GETCLIENTPROC	equ 34
SYSCALL_LOCK					equ 35
SYSCALL_UNLOCK				equ 36
SYSCALL_STARTTHREAD		equ 37
SYSCALL_GETTID				equ 38
SYSCALL_GETTHREADCNT	equ 39
SYSCALL_SEND					equ	40
SYSCALL_RECEIVE				equ	41
SYSCALL_HASMSG				equ 42
SYSCALL_SETDATARDA		equ	43
SYSCALL_GETCYCLES			equ 44
SYSCALL_SYNC					equ 45
SYSCALL_LINK					equ 46
SYSCALL_UNLINK				equ 47
SYSCALL_MKDIR					equ 48
SYSCALL_RMDIR					equ 49
SYSCALL_MOUNT					equ 50
SYSCALL_UNMOUNT				equ 51
SYSCALL_WAITCHILD			equ 52
SYSCALL_TELL					equ 53
SYSCALL_PIPE					equ 54
SYSCALL_GETCONF				equ 55
SYSCALL_VM86INT				equ 56
SYSCALL_GETWORK				equ 57
SYSCALL_ISTERM				equ 58
SYSCALL_JOIN					equ 59
SYSCALL_SUSPEND				equ 60
SYSCALL_RESUME				equ 61
SYSCALL_FSTAT					equ 62
SYSCALL_ADDREGION			equ 63

; the IRQ for syscalls
SYSCALL_IRQ						equ	0x30

; macros for the different syscall-types (void / ret-value, arg-count, error-handling)

%macro SYSC_VOID_0ARGS 2
[global %1]
%1:
	mov		eax,%2								; set syscall-number
	int		SYSCALL_IRQ
	ret
%endmacro

%macro SYSC_VOID_1ARGS 2
[global %1]
%1:
	mov		eax,%2								; set syscall-number
	mov		ecx,[esp + 4]					; set arg1
	int		SYSCALL_IRQ
	ret
%endmacro

%macro SYSC_RET_0ARGS 2
[global %1]
%1:
	mov		eax,%2								; set syscall-number
	int		SYSCALL_IRQ
	ret													; return-value is in eax
%endmacro

%macro SYSC_RET_0ARGS_ERR 2
[global %1]
%1:
	mov		eax,%2								; set syscall-number
	int		SYSCALL_IRQ
	test	ecx,ecx
	jz		.return								; no-error?
	mov		[errno],ecx						; store error-code
	mov		eax,ecx								; return error-code
.return:
	ret
%endmacro

%macro SYSC_RET_1ARGS_ERR 2
[global %1]
%1:
	mov		eax,%2								; set syscall-number
	mov		ecx,[esp + 4]					; set arg1
	int		SYSCALL_IRQ
	test	ecx,ecx
	jz		.return								; no-error?
	mov		[errno],ecx						; store error-code
	mov		eax,ecx								; return error-code
.return:
	ret
%endmacro

%macro SYSC_RET_2ARGS_ERR 2
[global %1]
%1:
	mov		eax,%2								; set syscall-number
	mov		ecx,[esp + 4]					; set arg1
	mov		edx,[esp + 8]					; set arg2
	int		SYSCALL_IRQ
	test	ecx,ecx
	jz		.return								; no-error?
	mov		[errno],ecx						; store error-code
	mov		eax,ecx								; return error-code
.return:
	ret
%endmacro

%macro SYSC_RET_3ARGS_ERR 2
[global %1]
%1:
	push	ebp
	mov		ebp,esp
	mov		ecx,[ebp + 8]					; set arg1
	mov		edx,[ebp + 12]				; set arg2
	push	DWORD [ebp + 16]			; push arg3
	mov		eax,%2								; set syscall-number
	int		SYSCALL_IRQ
	add		esp,4									; remove arg3
	test	ecx,ecx
	jz		.return								; no-error?
	mov		[errno],ecx						; store error-code
	mov		eax,ecx								; return error-code
.return:
	leave
	ret
%endmacro

%macro SYSC_RET_4ARGS_ERR 2
[global %1]
%1:
	push	ebp
	mov		ebp,esp
	mov		ecx,[ebp + 8]					; set arg1
	mov		edx,[ebp + 12]				; set arg2
	push	DWORD [ebp + 20]			; push arg4
	push	DWORD [ebp + 16]			; push arg3
	mov		eax,%2								; set syscall-number
	int		SYSCALL_IRQ
	add		esp,8									; remove arg3 and arg4
	test	ecx,ecx
	jz		.return								; no-error?
	mov		[errno],ecx						; store error-code
	mov		eax,ecx								; return error-code
.return:
	leave
	ret
%endmacro

%macro SYSC_RET_7ARGS_ERR 2
[global %1]
%1:
	push	ebp
	mov		ebp,esp
	mov		ecx,[ebp + 8]					; set arg1
	mov		edx,[ebp + 12]				; set arg2
	push	DWORD [ebp + 32]			; push arg7
	push	DWORD [ebp + 28]			; push arg6
	push	DWORD [ebp + 24]			; push arg5
	push	DWORD [ebp + 20]			; push arg4
	push	DWORD [ebp + 16]			; push arg3
	mov		eax,%2								; set syscall-number
	int		SYSCALL_IRQ
	add		esp,20								; remove args
	test	ecx,ecx
	jz		.return								; no-error?
	mov		[errno],ecx						; store error-code
	mov		eax,ecx								; return error-code
.return:
	leave
	ret
%endmacro
