/**
 * $Id: strtold.c 578 2010-03-29 15:54:22Z nasmussen $
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
#include <string.h>
#include <ctype.h>
#include <math.h>

static s32 getDigitVal(char c,u8 base);

long double strtold(const char *nptr,char **endptr) {
	bool neg = false;
	long double res = 0;
	/* skip leading space */
	while(isspace(*nptr))
		nptr++;
	/* handle +/- */
	if(*nptr == '-')
		neg = true;
	if(*nptr == '-' || *nptr == '+')
		nptr++;
	/* infinity */
	if(strncasecmp(nptr,"INFINITY",8) == 0) {
		nptr += 8;
		res = neg ? -INFINITY : INFINITY;
	}
	else if(strncasecmp(nptr,"INF",3) == 0) {
		nptr += 3;
		res = neg ? -INFINITY : INFINITY;
	}
	/* nan */
	else if(strncasecmp(nptr,"NAN",3) == 0) {
		nptr += 3;
		if(*nptr == '(') {
			while(isalnum(*nptr))
				nptr++;
			if(*nptr != ')')
				return 0;
		}
		res = NAN;
	}
	/* default */
	else {
		char c;
		u8 base = 10;
		if(strncasecmp(nptr,"0X",2) == 0) {
			nptr += 2;
			base = 16;
		}
		/* in front of "." */
		while((c = *nptr)) {
			s32 val = getDigitVal(c,base);
			if(val == -1)
				break;
			res = res * base + val;
			nptr++;
		}
		/* after "." */
		if(*nptr == '.') {
			u32 mul = base;
			nptr++;
			while((c = *nptr)) {
				s32 val = getDigitVal(c,base);
				if(val == -1)
					break;
				res += (long double)val / mul;
				mul *= base;
				nptr++;
			}
		}
		/* handle exponent */
		if((base == 10 && tolower(*nptr) == 'e') ||
			(base == 16 && tolower(*nptr) == 'p')) {
			s32 expo = 0;
			bool negExp = *++nptr == '-';
			if(*nptr == '-' || *nptr == '+')
				nptr++;
			while((c = *nptr)) {
				s32 val = getDigitVal(c,base);
				if(val == -1)
					break;
				expo = expo * base + val;
				nptr++;
			}
			base = base == 10 ? 10 : 2;
			if(negExp) {
				while(expo-- > 0)
					res /= base;
			}
			else {
				while(expo-- > 0)
					res *= base;
			}
		}
		if(neg)
			res = -res;
	}
	if(endptr)
		*endptr = (char*)nptr;
	return res;
}

static s32 getDigitVal(char c,u8 base) {
	if(isdigit(c))
		return c - '0';
	if(base == 16) {
		if(c >= 'a' && c <= 'f')
			return 10  + (c - 'a');
		else if(c >= 'A' && c <= 'F')
			return 10 + (c - 'A');
	}
	return -1;
}
