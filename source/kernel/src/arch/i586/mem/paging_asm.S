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

.global paging_enable
.global paging_flushTLB
.global paging_exchangePDir

# void paging_enable(void);
paging_enable:
	mov		%cr0,%eax
	or		$1 << 31,%eax			# set bit for paging-enabled
	mov		%eax,%cr0				# now paging is enabled :)
	ret

# void paging_flushTLB(void);
paging_flushTLB:
	mov		%cr3,%eax
	mov		%eax,%cr3
	ret

# void paging_exchangePDir(uintptr_t physAddr);
paging_exchangePDir:
	mov		4(%esp),%eax			# load page-dir-address
	mov		%eax,%cr3				# set page-dir
	ret
