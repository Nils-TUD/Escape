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

.set MULTIBOOT_MAGIC,				0x2BADB002
.set MAX_MMAP_ENTRIES,				10

.global gem5boot
.extern higherhalf

.section .initdata

mbInfo:
	// we have cmdline, mods and mmap
	.long	(1 << 2) | (1 << 3) | (1 << 6)		// flags
	.long	0									// mem_lower
	.long	0									// mem_upper
	.long	0									// boot_device
	.long	0									// cmdline
	.long	0									// mods_count
	.long	0									// mods_addr
	.long	0									// syms[0]
	.long	0									// syms[1]
	.long	0									// syms[2]
	.long	0									// syms[3]
	.long	0									// mmap_length
	.long	0									// mmap_addr

mbMap:
.rept 24 * MAX_MMAP_ENTRIES
	.byte 0
.endr

.code64

.section .init

gem5boot:
	// e820 entry:
	//  0.. 7: address
	//  8..15: length
	// 16..19: type

	// state: esi = real mode data
	// and we're already in long mode with paging enabled (identity mapping)

	// real mode data:
	// 0x228 : cmdline pointer
	// 0x1e8 : number of e820 map entries
	// 0x2d0 : e820 map pointer
	mov		$mbInfo,%rax
	mov		0x228(%rsi),%rcx		// cmdline
	mov		%rcx,4 * 4(%rax)

	mov		0x1e8(%rsi),%rcx
	lea		0x2d0(%rsi),%rbx
	mov		$mbMap,%rdx
	movl	%edx,12 * 4(%rax)		// mmap_addr
1:
	test	%rcx,%rcx
	je		2f
	movl	$20,(%rdx)				// size
	mov		(%rbx),%rdi
	movq	%rdi,4(%rdx)			// base_addr
	mov		8(%rbx),%rdi
	movq	%rdi,12(%rdx)			// length
	mov		16(%rbx),%rdi
	movl	%edi,20(%rdx)			// type (1 = available, other = reserved; for e820 and mb mmap)
	add		$24,%rdx				// to next mb mmap
	add		$20,%rbx				// to next e820 mmap
	sub		$1,%rcx
	jmp		1b
2:
	sub		$mbMap,%rdx
	movl	%edx,11 * 4(%rax)		// mmap_length
	mov		$mbInfo,%ebx			// store pointer to mb info in ebx, as required by mb spec
	mov		$MULTIBOOT_MAGIC,%eax	// store magic in eax, as required by mb spec

	mov		$higherhalf,%edi
	jmp		paging_setup
