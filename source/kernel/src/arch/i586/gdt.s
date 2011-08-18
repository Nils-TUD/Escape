/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

.global tss_load
.global gdt_flush
.global gdt_get

# void tss_load(size_t gdtOffset);
tss_load:
	ltr		4(%esp)					# load tss
	ret

# void gdt_flush(tGDTTable *gdt);
gdt_flush:
	mov		4(%esp),%eax			# load gdt-pointer into eax
	lgdt	(%eax)					# load gdt

	mov		$0x10,%eax				# set the value for the segment-registers
	mov		%eax,%ds				# reload segments
	mov		%eax,%es
	mov		%eax,%fs
	mov		%eax,%gs
	mov		%eax,%ss
	ljmp	$0x08,$2f				# reload code-segment via far-jump
2:
	ret								# we're done

# void gdt_get(sGDTTable *gdt);
gdt_get:
	mov		4(%esp),%eax
	sgdt	(%eax)
	ret
