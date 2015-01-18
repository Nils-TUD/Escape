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

#include <sys/common.h>
#include <sys/width.h>
#include <stdio.h>

#include "iobuf.h"

int bprintulpad(FILE *f,ullong u,uint base,uint pad,uint flags) {
	int count = 0;
	/* pad left - spaces */
	if(!(flags & FFL_PADRIGHT) && !(flags & FFL_PADZEROS) && pad > 0) {
		size_t width = getullwidth(u,base);
		if(pad > width)
			count += RETERR(bprintpad(f,pad - width,flags));
	}
	/* print base-prefix */
	PRINT_UNSIGNED_PREFIX(count,f,base,flags);
	/* pad left - zeros */
	if(!(flags & FFL_PADRIGHT) && (flags & FFL_PADZEROS) && pad > 0) {
		size_t width = getullwidth(u,base);
		if(pad > width)
			count += RETERR(bprintpad(f,pad - width,flags));
	}
	/* print number */
	if(flags & FFL_CAPHEX)
		count += RETERR(bprintul(f,u,base,hexCharsBig));
	else
		count += RETERR(bprintul(f,u,base,hexCharsSmall));
	/* pad right */
	if((flags & FFL_PADRIGHT) && (int)pad > count)
		count += RETERR(bprintpad(f,pad - count,flags));
	return count;
}
