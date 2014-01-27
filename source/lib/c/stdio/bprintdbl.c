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

#include <esc/common.h>
#include "iobuf.h"
#include <stdio.h>
#include <math.h>

int bprintdbl(FILE *f,double d,uint precision) {
	int c = 0;
	llong val = 0;

	/* Note: this is probably not a really good way of converting IEEE754-floating point numbers
	 * to string. But its simple and should be enough for my purposes :) */

	if(isnan(d)) {
		if(d < 0)
			c += RETERR(bputc(f,'-'));
		c += RETERR(bputs(f,"nan"));
	}
	else if(isinf(d)) {
		if(d < 0)
			c += RETERR(bputc(f,'-'));
		c += RETERR(bputs(f,"inf"));
	}
	else {
		val = (llong)d;
		c += bprintl(f,val);
		d -= val;
		if(d < 0)
			d = -d;
		RETERR(bputc(f,'.'));
		c++;
		while(precision-- > 0) {
			d *= 10;
			val = (llong)d;
			RETERR(bputc(f,(val % 10) + '0'));
			d -= val;
			c++;
		}
	}
	return c;
}
