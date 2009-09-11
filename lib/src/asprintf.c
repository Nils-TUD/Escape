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

#include <types.h>
#include <stddef.h>
#include <stdarg.h>
#include <asprintf.h>
#include <width.h>
#include <string.h>

#ifdef IN_KERNEL
#	include <mem/kheap.h>
#	define free(x)		kheap_free(x)
#	define malloc(x)	kheap_alloc(x)
#	define realloc(p,x)	kheap_realloc(p,x)
#else
#	include <esc/heap.h>
#endif

/* the minimum space-increase-size in util_vsprintf() */
#define SPRINTF_MIN_INC_SIZE	10

/* ensures that <buf> in util_vsprintf() has enough space for MAX(<width>,<pad>).
 * If necessary more space is allocated. If it fails false will be returned */
#define SPRINTF_INCREASE(width,pad) \
	if(buf->dynamic) { \
		u32 tmpLen = MAX((width),(pad)); \
		if(buf->len + tmpLen >= buf->size) { \
			buf->size += MAX(SPRINTF_MIN_INC_SIZE,1 + buf->len + tmpLen - buf->size); \
			buf->str = (char*)realloc(buf->str,buf->size * sizeof(char)); \
			if(!buf->str) { \
				buf->size = 0; \
				return false; \
			} \
		} \
	}

/* adds one char to <str> in util_vsprintf() and ensures that there is enough space for the char */
#define SPRINTF_ADD_CHAR(c) \
	SPRINTF_INCREASE(1,1); \
	*str++ = (c);

/* helper-functions */
static u32 asprintu(char *str,u32 n,u8 base);
static u32 asprintn(char *str,s32 n);
static u32 asprintfPad(char *str,char c,s32 count);

/* for ksprintf */
static char hexCharsBig[] = "0123456789ABCDEF";

u32 asprintfLen(const char *fmt,...) {
	u32 len;
	va_list ap;
	va_start(ap,fmt);
	len = avsprintfLen(fmt,ap);
	va_end(ap);
	return len;
}

u32 avsprintfLen(const char *fmt,va_list ap) {
	char c,b;
	char *s;
	u8 pad;
	s32 n;
	u32 u,width;
	bool readFlags;
	u8 base;
	u32 len = 0;

	while(1) {
		/* wait for a '%' */
		while((c = *fmt++) != '%') {
			/* finished? */
			if(c == '\0') {
				return len;
			}
			len++;
		}

		/* read flags */
		readFlags = true;
		while(readFlags) {
			switch(*fmt) {
				case '-':
				case '0':
					fmt++;
					break;
				default:
					readFlags = false;
					break;
			}
		}

		/* read pad-width */
		pad = 0;
		while(*fmt >= '0' && *fmt <= '9') {
			pad = pad * 10 + (*fmt - '0');
			fmt++;
		}

		/* determine format */
		switch(c = *fmt++) {
			/* signed integer */
			case 'd':
				n = va_arg(ap, s32);
				width = getnwidth(n);
				len += MAX(width,pad);
				break;
			/* unsigned integer */
			case 'b':
			case 'u':
			case 'o':
			case 'x':
				u = va_arg(ap, u32);
				base = c == 'o' ? 8 : (c == 'x' ? 16 : (c == 'b' ? 2 : 10));
				width = getuwidth(u,base);
				len += MAX(width,pad);
				break;
			/* string */
			case 's':
				s = va_arg(ap, char*);
				width = strlen(s);
				len += MAX(width,pad);
				break;
			/* character */
			case 'c':
				b = (char)va_arg(ap, u32);
				len++;
				break;
			/* all other */
			default:
				len++;
				break;
		}
	}
}

bool asprintf(sStringBuffer *buf,const char *fmt,...) {
	bool res;
	va_list ap;
	va_start(ap,fmt);
	res = avsprintf(buf,fmt,ap);
	va_end(ap);
	return res;
}

bool avsprintf(sStringBuffer *buf,const char *fmt,va_list ap) {
	char c,b,padchar;
	char *s,*str;
	u8 pad;
	s32 n;
	u32 u,width;
	bool readFlags,padRight;
	u8 base;

	if(buf->dynamic && buf->str == NULL) {
		buf->size = SPRINTF_MIN_INC_SIZE;
		buf->str = (char*)malloc(SPRINTF_MIN_INC_SIZE * sizeof(char));
		if(!buf->str)
			return false;
	}
	str = buf->str + buf->len;

	while(1) {
		/* wait for a '%' */
		while((c = *fmt++) != '%') {
			/* finished? */
			if(c == '\0') {
				SPRINTF_ADD_CHAR('\0');
				return true;
			}
			SPRINTF_ADD_CHAR(c);
			buf->len++;
		}

		/* read flags */
		padchar = ' ';
		padRight = false;
		readFlags = true;
		while(readFlags) {
			switch(*fmt) {
				case '-':
					padRight = true;
					fmt++;
					break;
				case '0':
					padchar = '0';
					fmt++;
					break;
				default:
					readFlags = false;
					break;
			}
		}

		/* read pad-width */
		pad = 0;
		while(*fmt >= '0' && *fmt <= '9') {
			pad = pad * 10 + (*fmt - '0');
			fmt++;
		}

		/* determine format */
		switch(c = *fmt++) {
			/* signed integer */
			case 'd':
				n = va_arg(ap, s32);
				width = getnwidth(n);
				SPRINTF_INCREASE(width,pad);
				if(!padRight && pad > 0)
					str += asprintfPad(str,padchar,pad - width);
				str += asprintn(str,n);
				if(padRight && pad > 0)
					str += asprintfPad(str,padchar,pad - width);
				buf->len += MAX(width,pad);
				break;
			/* unsigned integer */
			case 'b':
			case 'u':
			case 'o':
			case 'x':
				u = va_arg(ap, u32);
				base = c == 'o' ? 8 : (c == 'x' ? 16 : (c == 'b' ? 2 : 10));
				width = getuwidth(u,base);
				SPRINTF_INCREASE(width,pad);
				if(!padRight && pad > 0)
					str += asprintfPad(str,padchar,pad - width);
				str += asprintu(str,u,base);
				if(padRight && pad > 0)
					str += asprintfPad(str,padchar,pad - width);
				buf->len += MAX(width,pad);
				break;
			/* string */
			case 's':
				s = va_arg(ap, char*);
				width = strlen(s);
				SPRINTF_INCREASE(width,pad);
				if(!padRight && pad > 0)
					str += asprintfPad(str,padchar,pad - width);
				while(*s) {
					SPRINTF_ADD_CHAR(*s);
					s++;
				}
				if(padRight && pad > 0)
					str += asprintfPad(str,padchar,pad - width);
				buf->len += MAX(width,pad);
				break;
			/* character */
			case 'c':
				b = (char)va_arg(ap, u32);
				SPRINTF_ADD_CHAR(b);
				buf->len++;
				break;
			/* all other */
			default:
				SPRINTF_ADD_CHAR(c);
				buf->len++;
				break;
		}
	}
}

static u32 asprintu(char *str,u32 n,u8 base) {
	u32 c = 0;
	if(n >= base) {
		c += asprintu(str,n / base,base);
		str += c;
	}
	*str = hexCharsBig[n % base];
	return c + 1;
}

static u32 asprintn(char *str,s32 n) {
	u32 c = 0;
	if(n < 0) {
		*str = '-';
		n = -n;
		c++;
	}
	if(n >= 10)
		c += asprintn(str + c,n / 10);
	str += c;
	*str = '0' + n % 10;
	return c + 1;
}

static u32 asprintfPad(char *str,char c,s32 count) {
	char *start = str;
	while(count-- > 0) {
		*str++ = c;
	}
	return str - start;
}
