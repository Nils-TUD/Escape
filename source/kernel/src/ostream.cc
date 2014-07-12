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

#include <common.h>
#include <mem/cache.h>
#include <ostream.h>
#include <spinlock.h>
#include <sys/width.h>
#include <stdarg.h>
#include <string.h>

/* format flags */
#define FFL_PADRIGHT		1
#define FFL_FORCESIGN		2
#define FFL_SPACESIGN		4
#define FFL_PRINTBASE		8
#define FFL_PADZEROS		16
#define FFL_CAPHEX			32
#define FFL_LONGLONG		64
#define FFL_LONG			128
#define FFL_SIZE_T			256
#define FFL_INTPTR_T		512
#define FFL_OFF_T			1024

char OStream::hexCharsBig[] = "0123456789ABCDEF";
char OStream::hexCharsSmall[] = "0123456789abcdef";

void OStream::vwritef(const char *fmt,va_list ap) {
	char c,b;
	char *s;
	llong n;
	ullong u;
	size_t size;
	uint pad,base,flags;
	bool readFlags;
	int precision;

	while(1) {
		/* wait for a '%' */
		while((c = *fmt++) != '%') {
			/* escape */
			if(c == '\033' && escape(&fmt))
				continue;
			printc(c);
			/* finished? */
			if(c == '\0')
				return;
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
					pad = va_arg(ap, ulong);
					fmt++;
					break;
				case '|':
					pad = pipepad();
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
			if(*++fmt == '*') {
				precision = va_arg(ap, int);
				fmt++;
			}
			else {
				precision = 0;
				while(*fmt >= '0' && *fmt <= '9') {
					precision = precision * 10 + (*fmt - '0');
					fmt++;
				}
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
				flags |= FFL_SIZE_T;
				fmt++;
				break;
			case 'P':
				flags |= FFL_INTPTR_T;
				fmt++;
				break;
			case 'O':
				flags |= FFL_OFF_T;
				fmt++;
				break;
		}

		/* determine format */
		switch(c = *fmt++) {
			/* signed integer */
			case 'd':
			case 'i':
				if(flags & FFL_LONGLONG)
					n = va_arg(ap, llong);
				else if(flags & FFL_LONG)
					n = va_arg(ap, long);
				else if(flags & FFL_SIZE_T)
					n = va_arg(ap, ssize_t);
				else if(flags & FFL_INTPTR_T)
					n = va_arg(ap, intptr_t);
				else if(flags & FFL_OFF_T)
					n = va_arg(ap, off_t);
				else
					n = va_arg(ap, int);
				printnpad(n,pad,flags);
				break;

			/* pointer */
			case 'p':
				size = sizeof(uintptr_t);
				u = va_arg(ap, uintptr_t);
				flags |= FFL_PADZEROS;
				/* 2 hex-digits per byte and a ':' every 2 bytes */
				pad = size * 2 + size / 2;
				while(size > 0) {
					printupad((u >> (size * 8 - 16)) & 0xFFFF,16,4,flags);
					size -= 2;
					if(size > 0)
						printc(':');
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
				if(flags & FFL_LONGLONG)
					u = va_arg(ap, ullong);
				else if(flags & FFL_LONG)
					u = va_arg(ap, ulong);
				else if(flags & FFL_SIZE_T)
					u = va_arg(ap, size_t);
				else if(flags & FFL_INTPTR_T)
					u = va_arg(ap, uintptr_t);
				else if(flags & FFL_OFF_T)
					u = va_arg(ap, off_t);
				else
					u = va_arg(ap, uint);
				printupad(u,base,pad,flags);
				break;

			/* string */
			case 's':
				s = va_arg(ap, char*);
				size = s ? strlen(s) : 6;
				if(precision == -1)
					precision = size;
				if(pad > 0 && !(flags & FFL_PADRIGHT))
					printpad(pad - size,flags);
				n = prints(s ? s : "(null)",precision);
				if(pad > 0 && (flags & FFL_PADRIGHT))
					printpad(pad - n,flags);
				break;

			/* character */
			case 'c':
				b = (char)va_arg(ap, uint);
				printc(b);
				break;

			default:
				printc(c);
				break;
		}
	}
}

void OStream::printc(char c) {
	if(c && lineStart) {
		for(size_t i = 0; i < indent; ++i)
			writec('\t');
		lineStart = false;
	}
	writec(c);
	if(c == '\n')
		lineStart = true;
}

void OStream::printnpad(llong n,uint pad,uint flags) {
	int count = 0;
	/* pad left */
	if(!(flags & FFL_PADRIGHT) && pad > 0) {
		size_t width = getllwidth(n);
		if(n > 0 && (flags & (FFL_FORCESIGN | FFL_SPACESIGN)))
			width++;
		count += printpad(pad - width,flags);
	}
	/* print '+' or ' ' instead of '-' */
	if(n > 0) {
		if((flags & FFL_FORCESIGN)) {
			printc('+');
			count++;
		}
		else if(((flags) & FFL_SPACESIGN)) {
			printc(' ');
			count++;
		}
	}
	/* print number */
	count += printn(n);
	/* pad right */
	if((flags & FFL_PADRIGHT) && pad > 0)
		printpad(pad - count,flags);
}

void OStream::printupad(ullong u,uint base,uint pad,uint flags) {
	int count = 0;
	/* pad left - spaces */
	if(!(flags & FFL_PADRIGHT) && !(flags & FFL_PADZEROS) && pad > 0) {
		size_t width = getullwidth(u,base);
		count += printpad(pad - width,flags);
	}
	/* print base-prefix */
	if((flags & FFL_PRINTBASE)) {
		if(base == 16 || base == 8) {
			printc('0');
			count++;
		}
		if(base == 16) {
			char c = (flags & FFL_CAPHEX) ? 'X' : 'x';
			printc(c);
			count++;
		}
	}
	/* pad left - zeros */
	if(!(flags & FFL_PADRIGHT) && (flags & FFL_PADZEROS) && pad > 0) {
		size_t width = getullwidth(u,base);
		count += printpad(pad - width,flags);
	}
	/* print number */
	if(flags & FFL_CAPHEX)
		count += printu(u,base,hexCharsBig);
	else
		count += printu(u,base,hexCharsSmall);
	/* pad right */
	if((flags & FFL_PADRIGHT) && pad > 0)
		printpad(pad - count,flags);
}

int OStream::printpad(int count,uint flags) {
	int res = count;
	char c = flags & FFL_PADZEROS ? '0' : ' ';
	while(count-- > 0)
		printc(c);
	return res;
}

int OStream::printu(ullong n,uint base,char *chars) {
	int res = 0;
	if(n >= base)
		res += printu(n / base,base,chars);
	printc(chars[(n % base)]);
	return res + 1;
}

int OStream::printn(llong n) {
	int res = 0;
	if(n < 0) {
		printc('-');
		n = -n;
		res++;
	}

	if(n >= 10)
		res += printn(n / 10);
	printc('0' + n % 10);
	return res + 1;
}

int OStream::prints(const char *str,ssize_t len) {
	const char *begin = str;
	char c;
	while((len == -1 || len-- > 0) && (c = *str)) {
		printc(c);
		str++;
	}
	return str - begin;
}
