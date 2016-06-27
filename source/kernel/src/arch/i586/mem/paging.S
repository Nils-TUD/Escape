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

#include <mem/layout.h>
#include <sys/arch.h>

.global proc0TLPD
.global paging_setup

.section .initdata

// first page directory
.align PAGE_SIZE
proc0TLPD:
.rept PAGE_SIZE
	.byte 0
.endr

.code32

.section .init

// expects return-address in %edi
paging_setup:
	// note that we assume here that the kernel is not larger than 1 page-table (4MiB)!

	// setup paging
	mov		$proc0TLPD,%eax
	// virt -> phys
	and		$~KERNEL_AREA,%eax
	// put PDE in slot 0: PDE_EXISTS | PDE_GLOBAL | PDE_LARGE | PDE_WRITABLE | PDE_PRESENT
	movl	$0x00000383,(%eax)
	// put PDE in slot for kernel-area
	add		$(KERNEL_AREA >> (10 + 12 - 2)),%eax
	movl	$0x00000383,(%eax)
	// just to be sure
	and		$~(PAGE_SIZE - 1),%eax
	// set pagedir
	mov		%eax,%cr3
	// enable global pages and large pages
	mov		%cr4,%eax
	or		$0x90,%eax
	mov		%eax,%cr4
	// enable paging
	mov		%cr0,%eax
	or		$0x80000000,%eax
	mov		%eax,%cr0

	jmp		*%edi
