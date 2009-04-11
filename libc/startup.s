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
[extern ackSignal]

ALIGN 4

init:
	call	main

	; call exit with return-value of main
	push	eax
	call	exit

	; just to be sure
	jmp		$


; all signal-handler return to this "function" (address 0xd)
sigRetFunc:
	; ack signal so that the kernel knows that we accept another signal
	call	ackSignal
	; remove args
	add		esp,8
	; restore register
	pop		esi
	pop		edi
	pop		edx
	pop		ecx
	pop		ebx
	pop		eax
	; return to the instruction before the signal
	ret
