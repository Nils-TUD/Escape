/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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

# call the section ".init" to force ld to put this section at the beginning of the executable
.section .init

.extern higherhalf
.global loader
.global KernelStart

# the kernel entry point
KernelStart:
loader:
	# here is the trick: we load a GDT with a base address
	# of 0x40000000 for the code (0x08) and data (0x10) segments
	lgdt	(setupGDT)
	mov		$0x10,%ax
	mov		%ax,%ds
	mov		%ax,%es
	mov		%ax,%fs
	mov		%ax,%gs
	mov		%ax,%ss

	# jump to the higher half kernel
	ljmp	$0x08,$higherhalf

# our GDT for the setup-process
setupGDT:
	# GDT size
	.word		setupGDTEntries - setupGDTEntriesEnd - 1
	# Pointer to entries
	.long		setupGDTEntries

# the GDT-entries
setupGDTEntries:
	# null gate
	.long 0,0

	# code selector 0x08: base 0x40000000, limit 0xFFFFFFFF, type 0x9A, granularity 0xCF
	.byte 0xFF, 0xFF, 0, 0, 0, 0x9A, 0xCF, 0x40

	# data selector 0x10: base 0x40000000, limit 0xFFFFFFFF, type 0x92, granularity 0xCF
	.byte 0xFF, 0xFF, 0, 0, 0, 0x92, 0xCF, 0x40
setupGDTEntriesEnd:
