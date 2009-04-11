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

#include <esc/common.h>
#include <esc/mem.h>

/* the assembler-routine */
extern s32 _mapPhysical(u32 phys,u32 count);

/* just a convenience for the user because the return-value is negative if an error occurred */
/* since it will be mapped in the user-space (< 0x80000000) the MSB is never set */
void *mapPhysical(u32 phys,u32 count) {
	s32 err = _mapPhysical(phys,count);
	if(err < 0)
		return NULL;
	return (void*)err;
}
