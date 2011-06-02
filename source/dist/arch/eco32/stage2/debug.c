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
#include <esc/debug.h>
#include <esc/io.h>
#include <esc/dir.h>
#include <esc/width.h>
#include <esc/thread.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

static void debugPad(size_t count,uint flags);
static void debugStringn(char *s,uint precision);

#define DBG_FFL_PADZEROS	1

void dumpBytes(const void *addr,size_t byteCount) {
	size_t i = 0;
	uchar *ptr = (uchar*)addr;
	for(i = 0; byteCount-- > 0; i++) {
		if(i % 17 == 0)
			debugf("\n%08x: ",ptr);
		debugf("%02x ",*ptr);
		ptr++;
	}
}

void debugf(const char *fmt,...) {
	va_list ap;
	va_start(ap, fmt);
	vdebugf(fmt,ap);
	va_end(ap);
}

void vdebugf(const char *fmt,va_list ap) {
	char c,ch;
	uint pad,flags,precision;
	bool readFlags;
	int n;
	uint u;
	char *s;
	ullong l;

	while(1) {
		while((c = *fmt++) != '%') {
			if(c == '\0') {
				return;
			}
			debugChar(c);
		}

		/* read flags */
		flags = 0;
		pad = 0;
		readFlags = true;
		while(readFlags) {
			switch(*fmt) {
				case '0':
					flags |= DBG_FFL_PADZEROS;
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
		precision = 0;
		if(*fmt == '.') {
			fmt++;
			while(*fmt >= '0' && *fmt <= '9') {
				precision = precision * 10 + (*fmt - '0');
				fmt++;
			}
		}

		c = *fmt++;
		if(c == 'd') {
			n = va_arg(ap, int);
			/*if(pad > 0) {
				size_t width = getnwidth(n);
				debugPad(pad - width,flags);
			}*/
			debugInt(n);
		}
		else if(c == 'u' || c == 'o' || c == 'x') {
			uint base = c == 'o' ? 8 : (c == 'x' ? 16 : 10);
			u = va_arg(ap, int);
			/*if(pad > 0) {
				size_t width = getuwidth(u,base);
				debugPad(pad - width,flags);
			}*/
			debugUint(u,base);
		} else if(c == 's') {
			s = va_arg(ap, char*);
			/*if(pad > 0) {
				size_t width = precision > 0 ? MIN(precision,strlen(s)) : strlen(s);
				debugPad(pad - width,flags);
			}*/
			debugStringn(s,precision);
		} else if(c == 'c') {
			ch = va_arg(ap, int);
			debugChar(ch);
		} else if(c == 'b') {
			l = va_arg(ap, uint);
			debugUint(l, 2);
		} else {
			debugChar(c);
		}
	}
}

void debugInt(int n) {
	if(n < 0) {
		debugChar('-');
		n = -n;
	}
	debugUint(n,10);
}

void debugUint(uint n,uint base) {
	uint a;
	if((a = n / base))
		debugUint(a,base);
	debugChar("0123456789abcdef"[(int)(n % base)]);
}

void debugString(char *s) {
	debugStringn(s,0);
}

static void debugPad(size_t count,uint flags) {
	char c = flags & DBG_FFL_PADZEROS ? '0' : ' ';
	while(count-- > 0)
		debugChar(c);
}

static void debugStringn(char *s,uint precision) {
	while(*s) {
		debugChar(*s++);
		if(precision > 0 && --precision == 0)
			break;
	}
}
