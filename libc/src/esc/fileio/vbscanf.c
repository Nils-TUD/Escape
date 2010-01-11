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
#include <esc/fileio.h>
#include <ctype.h>
#include "fileiointern.h"

s32 vbscanf(sBuffer *buf,const char *fmt,va_list ap) {
	char *s = NULL,c,tc,rc = 0,length;
	const char *numTable = "0123456789abcdef";
	s32 *n,count = 0;
	u32 *u,x;
	u8 base;
	bool readFlags;
	bool shortPtr;
	bool discard;

	while(1) {
		/* wait for a '%' */
		while((c = *fmt++) != '%') {
			/* finished? */
			if(c == '\0')
				return count;
			if(c != (rc = bscanc(buf)) || rc == IO_EOF)
				return count;
		}

		/* skip whitespace */
		do {
			rc = bscanc(buf);
			if(rc == IO_EOF)
				return count;
		}
		while(isspace(rc));
		if(bscanback(buf,rc) == IO_EOF)
			return count;

		/* read flags */
		shortPtr = false;
		discard = false;
		readFlags = true;
		while(readFlags) {
			switch(*fmt) {
				case '*':
					discard = true;
					fmt++;
					break;
				case 'h':
					shortPtr = true;
					fmt++;
					break;
				default:
					readFlags = false;
					break;
			}
		}

		/* read length */
		length = 0;
		while(*fmt >= '0' && *fmt <= '9') {
			length = length * 10 + (*fmt - '0');
			fmt++;
		}
		if(length == 0)
			length = -1;

		/* determine format */
		switch(c = *fmt++) {
			/* integers */
			case 'b':
			case 'o':
			case 'x':
			case 'X':
			case 'u':
			case 'd': {
				bool neg = false;
				u32 val = 0;

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

				/* handle '-' */
				if(c == 'd') {
					rc = bscanc(buf);
					if(rc == IO_EOF)
						return count;
					if(rc == '-') {
						neg = true;
						length--;
					}
					else {
						if(bscanback(buf,rc) == IO_EOF)
							return count;
					}
				}

				/* read until an invalid char is found or the max length is reached */
				x = 0;
				while(length != 0) {
					rc = bscanc(buf);
					/* IO_EOF is ok if the stream ends and we've already got a valid number */
					if(rc == IO_EOF)
						break;
					tc = tolower(rc);
					if(tc >= '0' && tc <= numTable[base - 1]) {
						if(base > 10 && tc >= 'a')
							val = val * base + (10 + tc - 'a');
						else
							val = val * base + (tc - '0');
						if(length > 0)
							length--;
						x++;
					}
					else {
						if(bscanback(buf,rc) == IO_EOF)
							return count;
						break;
					}
				}

				/* valid number? */
				if(x == 0)
					return count;

				/* store value */
				if(!discard) {
					if(c == 'd') {
						n = va_arg(ap, s32*);
						if(shortPtr)
							*(s16*)n = neg ? -val : val;
						else
							*n = neg ? -val : val;
					}
					else {
						u = va_arg(ap, u32*);
						if(shortPtr)
							*(u16*)u = val;
						else
							*u = val;
					}
					count++;
				}
			}
			break;

			/* string */
			case 's':
				if(!discard)
					s = va_arg(ap, char*);

				while(length != 0) {
					rc = bscanc(buf);
					if(rc == IO_EOF)
						break;
					if(!isspace(rc)) {
						if(!discard)
							*s++ = rc;
						if(length > 0)
							length--;
					}
					else {
						if(bscanback(buf,rc) == IO_EOF)
							return count;
						break;
					}
				}

				if(!discard) {
					*s = '\0';
					count++;
				}
				break;

			/* character */
			case 'c':
				if(!discard)
					s = va_arg(ap, char*);

				rc = bscanc(buf);
				if(rc == IO_EOF)
					return count;

				if(!discard) {
					*s = rc;
					count++;
				}
				break;

			/* error */
			default:
				return count;
		}
	}
}
