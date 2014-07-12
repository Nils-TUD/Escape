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

#include <stddef.h>
#include <string.h>
#include <sys/width.h>
#include <assert.h>

#define MAX_DBL_LEN		64

char *ecvt(double number,int ndigits,int *decpt,int *sign) {
	static char res[MAX_DBL_LEN];
	char *str = res;
	/* put the stuff before decpt into the string */
	llong val = (llong)number;
	int c;
	size_t vwidth = getllwidth(val);
	assert(ndigits < MAX_DBL_LEN);
	/* getllwidth counts the '-' for negatives and '0' for zero */
	if(val <= 0)
		vwidth--;
	str = res + MIN(ndigits,(int)vwidth);
	if(val < 0) {
		val = -val;
		*sign = 1;
	}
	else
		*sign = 0;
	c = vwidth;
	while(val > 0) {
		if(c <= ndigits) {
			*--str = (val % 10) + '0';
			ndigits--;
		}
		val /= 10;
		c--;
	}

	/* store imaginary position of decimal point */
	*decpt = vwidth;

	/* now build the fraction digits */
	str = res + vwidth;
	if(number != 0) {
		number -= (llong)number;
		if(number < 0)
			number = -number;
		while(number > 0 && ndigits-- > 0) {
			number *= 10;
			val = (llong)number;
			*str++ = (val % 10) + '0';
			number -= val;
		}
		while(ndigits-- > 0)
			*str++ = '0';
	}
	*str = '\0';
	return res;
}
