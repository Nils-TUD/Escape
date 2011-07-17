;
; $Id$
; Copyright (C) 2008 - 2011 Nils Asmussen
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

[bits 16]
[org 0x7000]

trampolineStart:
	jmp		begin

apStartAddr:
	dd		0

begin:
	; load the GDT
	lgdt	[gdt]

	; enable protected mode
	mov		eax,cr0
	or		eax,0x1
	mov		cr0,eax

	; far-jump to protected mode
	jmp		0x08:dword 0xC000701D

[bits 32]
; 0xC000701D
protMode:
	; set segment regs
	mov		ax,0x10
	mov		ds,ax
	mov		es,ax
	mov		fs,ax
	mov		gs,ax
	mov		ss,ax

	mov		eax,dword [0xC0007002]
	jmp		eax

; our GDT for the setup-process
gdt:
	; GDT size
	dw		gdtEntries - gdtEntriesEnd - 1
	; Pointer to entries
	dd		gdtEntries

; the GDT-entries
gdtEntries:
	; null gate
	dd	 	0,0

	; code selector 0x08: base 0x40000000, limit 0xFFFFFFFF, type 0x9A, granularity 0xCF
	db		0xFF, 0xFF, 0, 0, 0, 0x9A, 0xCF, 0x40

	; data selector 0x10: base 0x40000000, limit 0xFFFFFFFF, type 0x92, granularity 0xCF
	db		0xFF, 0xFF, 0, 0, 0, 0x92, 0xCF, 0x40
gdtEntriesEnd:

trampolineEnd:
