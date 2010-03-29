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
[extern __libd_start]

ALIGN 4

%include "../libc/syscalls.s"

init:
	call	__libd_start
	call	main
	call	threadExit

; exit for additional threads
threadExit:
	push	eax
	call	exit
	; just to be sure
	jmp		$
	; c++-programs have address 0x2d for sigRetFunc. So we need to achieve this here, too
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
	nop
	nop
	nop
	nop
	nop

; all signal-handler return to this "function" (address 0x2d)
sigRetFunc:
	mov		eax,SYSCALL_ACKSIG
	int		SYSCALL_IRQ
	; never reached
