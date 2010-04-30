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
[extern init_tls]

ALIGN 4

%include "syscalls.s"

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
%ifndef SHAREDLIB
	[extern __libc_init]
	call	__libc_init
%endif
	call	main

threadExit:
	push	eax
	call	exit
	; just to be sure
	jmp		$

	; c++-programs have address 0x1039 for sigRetFunc. So we need to achieve this here, too
%ifdef SHAREDLIB
	nop
	nop
	nop
	nop
	nop
%endif
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop

; all signal-handler return to this "function" (address 0x1039)
sigRetFunc:
	mov		eax,SYSCALL_ACKSIG
	int		SYSCALL_IRQ
	; never reached
