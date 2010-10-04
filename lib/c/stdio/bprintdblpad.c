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
#include <esc/width.h>
#include "iobuf.h"
#include <stdio.h>

int bprintdblpad(FILE *f,double d,uint pad,uint flags,uint precision) {
	int count = 0;
	llong pre = (llong)d;
	/* pad left */
	if(!(flags & FFL_PADRIGHT) && pad > 0) {
		size_t width = getlwidth(pre) + precision + 1;
		if(pre > 0 && (flags & (FFL_FORCESIGN | FFL_SPACESIGN)))
			width++;
		if(pad > width)
			count += RETERR(bprintpad(f,pad - width,flags));
	}
	/* print '+' or ' ' instead of '-' */
	PRINT_SIGNED_PREFIX(count,f,pre,flags);
	/* print number */
	count += RETERR(bprintdbl(f,d,precision));
	/* pad right */
	if((flags & FFL_PADRIGHT) && (int)pad > count)
		count += RETERR(bprintpad(f,pad - count,flags));
	return count;
}
