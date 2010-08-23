/**
 * $Id: bprintulpad.c 332 2009-09-17 09:39:40Z nasmussen $
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
#include <esc/width.h>
#include <stdio.h>

s32 bprintulpad(FILE *f,u64 u,u8 base,u8 pad,u16 flags) {
	s32 count = 0;
	/* pad left - spaces */
	if(!(flags & FFL_PADRIGHT) && !(flags & FFL_PADZEROS) && pad > 0) {
		u32 width = getulwidth(u,base);
		count += RETERR(bprintpad(f,pad - width,flags));
	}
	/* print base-prefix */
	PRINT_UNSIGNED_PREFIX(count,f,base,flags);
	/* pad left - zeros */
	if(!(flags & FFL_PADRIGHT) && (flags & FFL_PADZEROS) && pad > 0) {
		u32 width = getulwidth(u,base);
		count += RETERR(bprintpad(f,pad - width,flags));
	}
	/* print number */
	if(flags & FFL_CAPHEX)
		count += RETERR(bprintul(f,u,base,hexCharsBig));
	else
		count += RETERR(bprintul(f,u,base,hexCharsSmall));
	/* pad right */
	if((flags & FFL_PADRIGHT) && pad > 0)
		count += RETERR(bprintpad(f,pad - count,flags));
	return count;
}
