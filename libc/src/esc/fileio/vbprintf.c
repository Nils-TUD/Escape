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
#include <string.h>
#include <stdarg.h>
#include "fileiointern.h"

s32 vbprintf(void *vbuf,const char *fmt,va_list ap) {
	sBuffer *buf = (sBuffer*)vbuf;
	char c,b,pad;
	char *s;
	s32 *ptr;
	s32 n;
	u32 u;
	s64 l;
	u64 ul;
	double d;
	float f;
	bool readFlags;
	u16 flags;
	s16 precision;
	u8 width,base;
	s32 count = 0;

	while(1) {
		/* wait for a '%' */
		while((c = *fmt++) != '%') {
			/* finished? */
			if(c == '\0')
				return count;
			if(bprintc(buf,c) == IO_EOF)
				return count;
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
					pad = (u8)va_arg(ap, u32);
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
					l = va_arg(ap, u32);
					l |= ((s64)va_arg(ap, s32)) << 32;
					count += bprintlpad(buf,l,pad,flags);
				}
				else {
					n = va_arg(ap, s32);
					if(flags & FFL_SHORT)
						n &= 0xFFFF;
					count += bprintnpad(buf,n,pad,flags);
				}
				break;

			/* pointer */
			case 'p':
				u = va_arg(ap, u32);
				flags |= FFL_PADZEROS;
				pad = 9;
				count += bprintupad(buf,u >> 16,16,pad - 5,flags);
				if(bprintc(buf,':') == IO_EOF)
					return count;
				count++;
				count += bprintupad(buf,u & 0xFFFF,16,4,flags);
				break;

			/* number of chars written so far */
			case 'n':
				ptr = va_arg(ap, s32*);
				*ptr = count;
				break;

			/* floating points */
			case 'f':
				precision = precision == -1 ? 6 : precision;
				if(flags & FFL_LONG) {
					d = va_arg(ap, double);
					count += bprintdblpad(buf,d,pad,flags,precision);
				}
				else {
					/* 'float' is promoted to 'double' when passed through '...' */
					f = va_arg(ap, double);
					count += bprintdblpad(buf,f,pad,flags,precision);
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
					ul = va_arg(ap, u32);
					ul |= ((u64)va_arg(ap, u32)) << 32;
					count += bprintulpad(buf,ul,base,pad,flags);
				}
				else {
					u = va_arg(ap, u32);
					if(flags & FFL_SHORT)
						u &= 0xFFFF;
					count += bprintupad(buf,u,base,pad,flags);
				}
				break;

			/* string */
			case 's':
				s = va_arg(ap, char*);
				if(pad > 0 && !(flags & FFL_PADRIGHT)) {
					width = precision == -1 ? strlen(s) : (u32)precision;
					count += bprintpad(buf,pad - width,flags);
				}
				if(precision != -1) {
					b = s[precision];
					s[precision] = '\0';
				}
				n = bprints(buf,s);
				if(precision != -1)
					s[precision] = b;
				if(pad > 0 && (flags & FFL_PADRIGHT))
					count += bprintpad(buf,pad - n,flags);
				count += n;
				break;

			/* character */
			case 'c':
				b = (char)va_arg(ap, u32);
				if(bprintc(buf,b) == IO_EOF)
					return count;
				count++;
				break;

			default:
				if(bprintc(buf,c) == IO_EOF)
					return count;
				count++;
				break;
		}
	}
}
