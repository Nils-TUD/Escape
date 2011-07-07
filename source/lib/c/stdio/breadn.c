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

#include <esc/common.h>
#include "iobuf.h"
#include <stdio.h>
#include <ctype.h>

int breadn(FILE *f,int *num,size_t length,int c) {
	bool neg = false;
	int val = 0;
	size_t digits = 0;
	uint base;
	/* handle '-' */
	if(c == 'd') {
		int rc = RETERR(bgetc(f));
		if(rc == '-') {
			neg = true;
			length--;
		}
		else
			bback(f);
	}

	/* determine base */
	switch(c) {
		case 'b':
			base = 2;
			break;
		case 'o':
			base = 8;
			break;
		case 'x':
		case 'X':
			base = 16;
			break;
		default:
			base = 10;
			break;
	}

	/* read until an invalid char is found or the max length is reached */
	while(length != 0) {
		int tc = bgetc(f);
		if(tc == EOF)
			break;
		tc = tolower(tc);
		if(tc >= '0' && tc <= hexCharsSmall[base - 1]) {
			if(base > 10 && tc >= 'a')
				val = val * base + (10 + tc - 'a');
			else
				val = val * base + (tc - '0');
			if(length > 0)
				length--;
			digits++;
		}
		else {
			bback(f);
			break;
		}
	}
	if(neg)
		val = -val;
	*num = val;
	if(digits > 0)
		return 0;
	return EOF;
}
