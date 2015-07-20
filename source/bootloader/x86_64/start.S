/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

.global _start
.extern loader
.extern mbi

.section .data

.rept 4096
	.byte 0
.endr
bootStack:

.section .text
_start:
	mov		$0,%rbp
	mov		$bootStack,%rsp

	// real mode data (pointer in %rsi):
	// 0x228 : cmdline pointer
	// 0x1e8 : number of e820 map entries
	// 0x2d0 : e820 map pointer
	mov		0x228(%rsi),%rdi	// arg1
	lea		0x2d0(%rsi),%rdx	// arg3
	mov		0x1e8(%rsi),%rsi	// arg2
	call	loader

	// prepare registers according to multiboot standard
	mov		%rax,%rcx
	mov		$0x2BADB002,%eax
	mov		$mbi,%ebx
	// TODO we are the BSP
	mov		$1,%esi
	// TODO jump to higherhalf after paging_setup
	mov		$0x100082,%edi
	// TODO jump to paging_setup
	mov		$0x1000b9,%rcx
	jmp		*%rcx
