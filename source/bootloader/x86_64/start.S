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

// TODO these adresse from the escape binary are hardcoded at the moment
// because gem5 already starts in longmode, so that we have to tweak
// the path through the boot code of escape a bit
#define HIGHERHALF  0x100082
#define PG_SETUP   	0x1000d3

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

	// we are the BSP
	mov		$1,%esi
	// jump to higherhalf after paging_setup
	mov		$HIGHERHALF,%edi
	// jump to paging_setup
	mov		$PG_SETUP,%rcx
	jmp		*%rcx
