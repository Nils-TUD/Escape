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

ALIGN 4

%include "../../../libc/syscalls.s"

init:
	; load modules first
	mov		eax,SYSCALL_LOADMODS
	int		SYSCALL_IRQ

	; now replace with init
	mov		ecx,progName						; set path
	mov		edx,argp								; set arguments
	mov		eax,SYSCALL_EXEC				; set syscall-number
	int		SYSCALL_IRQ

	; we should not reach this
	jmp		$

argp:
	dd		progName
	dd		0

progName:
	db		"/bin/init"
	db		0
