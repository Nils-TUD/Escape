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

#include <arch/x86_64/mem/layout.h>
#include <esc/arch.h>

.global proc0TLPD
.global paging_setup

.section .initdata
// PML4
.align PAGE_SIZE
proc0TLPD:
.rept PAGE_SIZE
	.byte 0
.endr

proc0PDPT:
.rept PAGE_SIZE
	.byte 0
.endr

proc0PD:
.rept PAGE_SIZE
	.byte 0
.endr

.code32

.section .init

// expects return-address in %edi
paging_setup:
	// note that we assume here that the kernel is not larger than 3 page-tables (6 MiB)!

	// setup paging structures
	mov		$proc0TLPD,%eax
	// put PDPT in slot for kernel-area
	add		$(((KERNEL_START & 0xFFFFFFFFFFFF) >> (9 * 3 + 12)) << 3),%eax
	mov		$proc0PDPT,%ecx
	// PDE_EXISTS | PDE_GLOBAL | PDE_WRITABLE | PDE_PRESENT
	or		$0x203,%ecx
	mov		%ecx,(%eax)
	// put PDPTE for kernel-area in PDPT
	mov		$proc0PDPT,%eax
	add		$(((KERNEL_START & 0x7FFFFFFFFF) >> (9 * 2 + 12)) << 3),%eax
	// PDE_EXISTS | PDE_GLOBAL | PDE_WRITABLE | PDE_PRESENT
	mov		$proc0PD,%ecx
	or		$0x203,%ecx
	movl	%ecx,(%eax)
	// put PDEs for kernel-area in PD
	mov		$proc0PD,%eax
	add		$(((KERNEL_START & 0x3FFFFFFF) >> (9 * 1 + 12)) << 3),%eax
	// PDE_EXISTS | PDE_GLOBAL | PDE_LARGE | PDE_WRITABLE | PDE_PRESENT
	movl	$0x00000383,(%eax)
	movl	$0x00200383,8(%eax)
	movl	$0x00400383,16(%eax)
	// now map it at 0x0, too
	mov		$proc0TLPD,%eax
	mov		$proc0PDPT,%ecx
	or		$0x203,%ecx
	mov		%ecx,(%eax)
	mov		$proc0PDPT,%eax
	mov		$proc0PD,%ecx
	or		$0x203,%ecx
	mov		%ecx,(%eax)

	// set PML4
	mov		$proc0TLPD,%eax
	mov		%eax,%cr3
	// enable global pages, large pages and PAE
	mov		%cr4,%eax
	or		$0xB0,%eax
	mov		%eax,%cr4
	// set EFER.LME = 1 and EFER.SCE = 1
	mov		$0xC0000080,%ecx
	rdmsr
	or		$0x101,%eax
	wrmsr
	// enable paging
	mov		%cr0,%eax
	or		$0x80000000,%eax
	mov		%eax,%cr0

	jmp		*%edi
