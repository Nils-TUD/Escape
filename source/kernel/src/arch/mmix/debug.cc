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

#include <sys/common.h>
#include <sys/debug.h>
#include <esc/width.h>
#include <string.h>
#include <stdarg.h>

#define DBG_FFL_PADZEROS	1

static void debugPad(size_t count,uint flags);
static void debugsn(const char *s,uint precision);

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
	long n;
	ulong u;
	char *s;
	ullong l;

	while(1) {
		while((c = *fmt++) != '%') {
			if(c == '\0') {
				return;
			}
			debugc(c);
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
			n = va_arg(ap, long);
			if(pad > 0) {
				size_t width = getnwidth(n);
				debugPad(pad - width,flags);
			}
			debugi(n);
		}
		else if(c == 'u' || c == 'o' || c == 'x') {
			uint base = c == 'o' ? 8 : (c == 'x' ? 16 : 10);
			u = va_arg(ap, long);
			if(pad > 0) {
				size_t width = getuwidth(u,base);
				debugPad(pad - width,flags);
			}
			debugu(u,base);
		} else if(c == 's') {
			s = va_arg(ap, char*);
			if(pad > 0) {
				size_t width = precision > 0 ? MIN(precision,strlen(s)) : strlen(s);
				debugPad(pad - width,flags);
			}
			debugsn(s,precision);
		} else if(c == 'c') {
			ch = va_arg(ap, int);
			debugc(ch);
		} else if(c == 'b') {
			l = va_arg(ap, uint);
			debugu(l, 2);
		} else {
			debugc(c);
		}
	}
}

void debugi(long n) {
	if(n < 0) {
		debugc('-');
		n = -n;
	}
	debugu(n,10);
}

void debugu(ulong n,ulong base) {
	ulong a;
	if((a = n / base))
		debugu(a,base);
	debugc("0123456789abcdef"[n % base]);
}

void debugs(const char *s) {
	debugsn(s,0);
}

static void debugPad(size_t count,uint flags) {
	char c = flags & DBG_FFL_PADZEROS ? '0' : ' ';
	while(count-- > 0)
		debugc(c);
}

static void debugsn(const char *s,uint precision) {
	while(*s) {
		debugc(*s++);
		if(precision > 0 && --precision == 0)
			break;
	}
}
