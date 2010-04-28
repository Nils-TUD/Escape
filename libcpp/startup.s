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
[global init]
[extern main]
[extern exit]
[extern __libcpp_start]
[extern __cxa_finalize]
[extern getThreadCount]
[extern init_tls]

ALIGN 4

%include "../libc/syscalls.s"

;  Initial stack:
;  +------------------+  <- top
;  |     arguments    |
;  |        ...       |
;  +------------------+
;  |       argv       |
;  +------------------+
;  |       argc       |
;  +------------------+
;  |     TLSSize      |  0 if not present
;  +------------------+
;  |     TLSStart     |  0 if not present
;  +------------------+
;  |    entryPoint    |  0 for initial thread, thread-entrypoint for others
;  +------------------+

init:
	; first call init_tls(entryPoint,TLSStart,TLSSize)
	call	init_tls
	; remove args from stack
	add		esp,12
	; it returns the entrypoint; 0 if we're the initial thread
	test	eax,eax
	je		initialThread
	; we're an additional thread, so call the desired function
	call	eax
	jmp		threadExit

	; initial thread calls main
initialThread:
	call	main

threadExit:
	; first, save return-value of main
	push	eax
	; we want to call global destructors just for the last thread
	call	getThreadCount
	mov		ebx,1
	cmp		eax,ebx
	jne		threadExitFinish
	; call global destructors
	call	__cxa_finalize
threadExitFinish:
	call	exit
	; just to be sure
	jmp		$

; all signal-handler return to this "function" (address 0x1039)
sigRetFunc:
	mov		eax,SYSCALL_ACKSIG
	int		SYSCALL_IRQ
	; never reached
