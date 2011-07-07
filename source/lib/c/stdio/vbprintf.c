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
#include "iobuf.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

int vbprintf(FILE *f,const char *fmt,va_list ap) {
	char c,b,pad;
	char *s;
	int *ptr;
	int n;
	uint u;
	long l;
	ulong ul;
	llong ll;
	ullong ull;
	uintptr_t uiptr;
	double d;
	float fl;
	bool readFlags;
	uint flags,width,base;
	size_t size;
	ssize_t ssize;
	int precision;
	int count = 0;

	while(1) {
		/* wait for a '%' */
		while((c = *fmt++) != '%') {
			/* finished? */
			if(c == '\0')
				return count;
			RETERR(bputc(f,c));
			count++;
		}

		/* read flags */
		flags = 0;
		pad = 0;
		readFlags = true;
		while(readFlags) {
			switch(*fmt) {
				case '-':
					flags |= FFL_PADRIGHT;
					fmt++;
					break;
				case '+':
					flags |= FFL_FORCESIGN;
					fmt++;
					break;
				case ' ':
					flags |= FFL_SPACESIGN;
					fmt++;
					break;
				case '#':
					flags |= FFL_PRINTBASE;
					fmt++;
					break;
				case '0':
					flags |= FFL_PADZEROS;
					fmt++;
					break;
				case '*':
					pad = va_arg(ap, uint);
					fmt++;
					break;
				default:
					readFlags = false;
					break;
			}
		}

		/* read pad-width */
		if(pad == 0) {
			while(*fmt >= '0' && *fmt <= '9') {
				pad = pad * 10 + (*fmt - '0');
				fmt++;
			}
		}

		/* read precision */
		precision = -1;
		if(*fmt == '.') {
			fmt++;
			precision = 0;
			while(*fmt >= '0' && *fmt <= '9') {
				precision = precision * 10 + (*fmt - '0');
				fmt++;
			}
		}

		/* read length */
		switch(*fmt) {
			case 'l':
				flags |= FFL_LONG;
				fmt++;
				break;
			case 'L':
				flags |= FFL_LONGLONG;
				fmt++;
				break;
			case 'z':
				flags |= FFL_SIZE;
				fmt++;
				break;
			case 'h':
				flags |= FFL_SHORT;
				fmt++;
				break;
		}

		/* determine format */
		switch(c = *fmt++) {
			/* signed integer */
			case 'd':
			case 'i':
				if(flags & FFL_LONGLONG) {
					ll = va_arg(ap, llong);
					count += RETERR(bprintlpad(f,ll,pad,flags));
				}
				else if(flags & FFL_LONG) {
					l = va_arg(ap, long);
					count += RETERR(bprintlpad(f,l,pad,flags));
				}
				else if(flags & FFL_SIZE) {
					ssize = va_arg(ap, ssize_t);
					count += RETERR(bprintlpad(f,ssize,pad,flags));
				}
				else {
					n = va_arg(ap, int);
					if(flags & FFL_SHORT)
						n &= 0xFFFF;
					count += RETERR(bprintnpad(f,n,pad,flags));
				}
				break;

			/* pointer */
			case 'p':
				size = sizeof(uintptr_t);
				uiptr = va_arg(ap, uintptr_t);
				flags |= FFL_PADZEROS;
				/* 2 hex-digits per byte and a ':' every 2 bytes */
				pad = size * 2 + size / 2;
				while(size > 0) {
					count += RETERR(bprintupad(f,(uiptr >> (size * 8 - 16)) & 0xFFFF,16,4,flags));
					size -= 2;
					if(size > 0) {
						RETERR(bputc(f,':'));
						count++;
					}
				}
				break;

			/* number of chars written so far */
			case 'n':
				ptr = va_arg(ap, int*);
				*ptr = count;
				break;

			/* floating points */
			case 'f':
				precision = precision == -1 ? 6 : precision;
				if(flags & FFL_LONG) {
					d = va_arg(ap, double);
					count += RETERR(bprintdblpad(f,d,pad,flags,precision));
				}
				else {
					/* 'float' is promoted to 'double' when passed through '...' */
					fl = va_arg(ap, double);
					count += RETERR(bprintdblpad(f,fl,pad,flags,precision));
				}
				break;

			/* unsigned integer */
			case 'b':
			case 'u':
			case 'o':
			case 'x':
			case 'X':
				base = c == 'o' ? 8 : ((c == 'x' || c == 'X') ? 16 : (c == 'b' ? 2 : 10));
				if(c == 'X')
					flags |= FFL_CAPHEX;
				if(flags & FFL_LONGLONG) {
					ull = va_arg(ap, ullong);
					count += RETERR(bprintulpad(f,ull,base,pad,flags));
				}
				else if(flags & FFL_LONG) {
					ul = va_arg(ap, ulong);
					count += RETERR(bprintulpad(f,ul,base,pad,flags));
				}
				else if(flags & FFL_SIZE) {
					size = va_arg(ap, size_t);
					count += RETERR(bprintulpad(f,size,base,pad,flags));
				}
				else {
					u = va_arg(ap, uint);
					if(flags & FFL_SHORT)
						u &= 0xFFFF;
					count += RETERR(bprintupad(f,u,base,pad,flags));
				}
				break;

			/* string */
			case 's':
				b = 0;
				s = va_arg(ap, char*);
				if(pad > 0 && !(flags & FFL_PADRIGHT)) {
					width = precision == -1 ? strlen(s) : (uint)precision;
					if(pad > (int)width)
						count += RETERR(bprintpad(f,pad - width,flags));
				}
				if(precision != -1) {
					b = s[precision];
					s[precision] = '\0';
				}
				n = RETERR(bputs(f,s));
				if(precision != -1)
					s[precision] = b;
				if(pad > n && (flags & FFL_PADRIGHT))
					count += RETERR(bprintpad(f,pad - n,flags));
				count += n;
				break;

			/* character */
			case 'c':
				b = (char)va_arg(ap, uint);
				RETERR(bputc(f,b));
				count++;
				break;

			default:
				RETERR(bputc(f,c));
				count++;
				break;
		}
	}
}
