/**
 * $Id: outputstream.c 646 2010-05-04 15:19:36Z nasmussen $
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
#include <esc/mem/heap.h>
#include <esc/io/outputstream.h>
#include <string.h>
#include <width.h>
#include <stdarg.h>

/* format flags */
#define FFL_PADRIGHT		1
#define FFL_FORCESIGN		2
#define FFL_SPACESIGN		4
#define FFL_PRINTBASE		8
#define FFL_PADZEROS		16
#define FFL_CAPHEX			32
#define FFL_SHORT			64
#define FFL_LONG			128
#define FFL_LONGLONG		256

/* prints ' ' or '+' in front of n if positive and depending on the flags */
#define PRINT_SIGNED_PREFIX(count,s,n,flags) \
	if((n) > 0) { \
		if(((flags) & FFL_FORCESIGN)) { \
			(s)->writec((s),'+'); \
			(count)++; \
		} \
		else if(((flags) & FFL_SPACESIGN)) { \
			(s)->writec((s),' '); \
			(count)++; \
		} \
	}

/* prints '0', 'X' or 'x' as prefix depending on the flags and base */
#define PRINT_UNSIGNED_PREFIX(count,s,base,flags) \
	if(((flags) & FFL_PRINTBASE)) { \
		if((base) == 16 || (base) == 8) { \
			(s)->writec((s),'0'); \
			(count)++; \
		} \
		if((base) == 16) { \
			(s)->writec((s),((flags) & FFL_CAPHEX) ? 'X' : 'x'); \
			(count)++; \
		} \
	}

static s32 ostream_writes(sOStream *s,const char *str);
static s32 ostream_writeln(sOStream *s,const char *str);
static s32 ostream_writen(sOStream *s,s32 n);
static s32 ostream_writel(sOStream *s,s64 l);
static s32 ostream_writeu(sOStream *s,u32 u,u8 base);
static s32 ostream_writeul(sOStream *s,u64 u,u8 base);
static s32 ostream_writedbl(sOStream *s,double d,u32 precision);
static s32 ostream_writef(sOStream *s,const char *fmt,...);
static s32 ostream_vwritef(sOStream *s,const char *fmt,va_list ap);
static s32 ostream_writeuchars(sOStream *s,u32 u,u8 base,const char *hexchars);
static s32 ostream_writeulchars(sOStream *s,u64 u,u8 base,const char *hexchars);
static s32 ostream_writenpad(sOStream *s,s32 n,u8 pad,u16 flags);
static s32 ostream_writelpad(sOStream *s,s64 n,u8 pad,u16 flags);
static s32 ostream_writeupad(sOStream *s,u32 u,u8 base,u8 pad,u16 flags);
static s32 ostream_writeulpad(sOStream *s,u64 u,u8 base,u8 pad,u16 flags);
s32 ostream_writedblpad(sOStream *s,double d,u8 pad,u16 flags,u32 precision);
static s32 ostream_writepad(sOStream *s,s32 count,u16 flags);

static const char *hexCharsSmall = "0123456789abcdef";
static const char *hexCharsBig = "0123456789ABCDEF";

sOStream *ostream_open() {
	sOStream *s = (sOStream*)heap_alloc(sizeof(sOStream));
	s->writes = ostream_writes;
	s->writen = ostream_writen;
	s->writel = ostream_writel;
	s->writeu = ostream_writeu;
	s->writeul = ostream_writeul;
	s->writedbl = ostream_writedbl;
	s->writeln = ostream_writeln;
	s->writef = ostream_writef;
	s->vwritef = ostream_vwritef;
	return s;
}

void ostream_close(sOStream *s) {
	heap_free(s);
}

static s32 ostream_writes(sOStream *s,const char *str) {
	char c;
	char *start = (char*)str;
	while((c = *str)) {
		s->writec(s,c);
		str++;
	}
	return str - start;
}

static s32 ostream_writeln(sOStream *s,const char *str) {
	s32 c = s->writes(s,str);
	c += s->writec(s,'\n');
	return c;
}

static s32 ostream_writen(sOStream *s,s32 n) {
	u32 c = 0;
	if(n < 0) {
		s->writec(s,'-');
		n = -n;
		c++;
	}

	if(n >= 10)
		c += ostream_writen(s,n / 10);
	s->writec(s,'0' + n % 10);
	return c + 1;
}

static s32 ostream_writel(sOStream *s,s64 l) {
	s32 c = 0;
	if(l < 0) {
		s->writec(s,'-');
		l = -l;
		c++;
	}
	if(l >= 10)
		c += ostream_writel(s,l / 10);
	s->writec(s,(l % 10) + '0');
	return c + 1;
}

static s32 ostream_writeu(sOStream *s,u32 u,u8 base) {
	return ostream_writeuchars(s,u,base,hexCharsSmall);
}

static s32 ostream_writeul(sOStream *s,u64 u,u8 base) {
	return ostream_writeulchars(s,u,base,hexCharsSmall);
}

static s32 ostream_writedbl(sOStream *s,double d,u32 precision) {
	s32 c = 0;
	s64 val = 0;

	/* Note: this is probably not a really good way of converting IEEE754-floating point numbers
	 * to string. But its simple and should be enough for my purposes :) */

	val = (s64)d;
	c += s->writel(s,val);
	d -= val;
	if(d < 0)
		d = -d;
	s->writec(s,'.');
	c++;
	while(precision-- > 0) {
		d *= 10;
		val = (s64)d;
		s->writec(s,(val % 10) + '0');
		d -= val;
		c++;
	}
	return c;
}

static s32 ostream_writef(sOStream *s,const char *fmt,...) {
	va_list ap;
	s32 res;
	va_start(ap,fmt);
	res = ostream_vwritef(s,fmt,ap);
	va_end(ap);
	return res;
}

static s32 ostream_vwritef(sOStream *s,const char *fmt,va_list ap) {
	char c;
	bool readFlags;
	u16 flags;
	s16 precision;
	u8 pad;
	char *str,b;
	s32 n;
	u32 u;
	s64 l;
	u64 ul;
	s32 *ptr;
	double d;
	u8 width,base;
	s32 count = 0;

	while(1) {
		/* wait for a '%' */
		while((c = *fmt++) != '%') {
			/* finished? */
			s->writec(s,c);
			if(c == '\0')
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

		/* format */
		switch((c = *fmt++)) {
			/* signed integer */
			case 'd':
			case 'i':
				if(flags & FFL_LONGLONG) {
					l = va_arg(ap, u32);
					l |= ((s64)va_arg(ap, s32)) << 32;
					count += ostream_writelpad(s,l,pad,flags);
				}
				else {
					n = va_arg(ap, s32);
					if(flags & FFL_SHORT)
						n &= 0xFFFF;
					count += ostream_writenpad(s,n,pad,flags);
				}
				break;

			/* pointer */
			case 'p':
				u = va_arg(ap, u32);
				flags |= FFL_PADZEROS;
				pad = 9;
				count += ostream_writeupad(s,u >> 16,16,pad - 5,flags);
				s->writec(s,':');
				count++;
				count += ostream_writeupad(s,u & 0xFFFF,16,4,flags);
				break;

			/* number of chars written so far */
			case 'n':
				ptr = va_arg(ap, s32*);
				*ptr = count;
				break;

			/* floating points */
			case 'f':
				precision = precision == -1 ? 6 : precision;
				/* 'float' is promoted to 'double' when passed through '...' */
				d = va_arg(ap, double);
				count += ostream_writedblpad(s,d,pad,flags,precision);
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
					count += ostream_writeulpad(s,ul,base,pad,flags);
				}
				else {
					u = va_arg(ap, u32);
					if(flags & FFL_SHORT)
						u &= 0xFFFF;
					count += ostream_writeupad(s,u,base,pad,flags);
				}
				break;

			/* string */
			case 's':
				str = va_arg(ap, char*);
				if(pad > 0 && !(flags & FFL_PADRIGHT)) {
					width = precision == -1 ? strlen(str) : (u32)precision;
					count += ostream_writepad(s,pad - width,flags);
				}
				if(precision != -1) {
					b = str[precision];
					str[precision] = '\0';
				}
				n = s->writes(s,str);
				if(precision != -1)
					str[precision] = b;
				if(pad > 0 && (flags & FFL_PADRIGHT))
					count += ostream_writepad(s,pad - n,flags);
				count += n;
				break;

			/* character */
			case 'c':
				b = (char)va_arg(ap, u32);
				s->writec(s,b);
				count++;
				break;

			default:
				s->writec(s,c);
				count++;
				break;
		}
	}
}

static s32 ostream_writeuchars(sOStream *s,u32 u,u8 base,const char *hexchars) {
	s32 c = 0;
	if(u >= base)
		c += ostream_writeuchars(s,u / base,base,hexchars);
	s->writec(s,hexchars[(u % base)]);
	return c + 1;
}

static s32 ostream_writeulchars(sOStream *s,u64 u,u8 base,const char *hexchars) {
	s32 c = 0;
	if(u >= base)
		c += ostream_writeulchars(s,u / base,base,hexchars);
	s->writec(s,hexchars[(u % base)]);
	return c + 1;
}

static s32 ostream_writenpad(sOStream *s,s32 n,u8 pad,u16 flags) {
	s32 count = 0;
	/* pad left */
	if(!(flags & FFL_PADRIGHT) && pad > 0) {
		u32 width = getnwidth(n);
		if(n > 0 && (flags & (FFL_FORCESIGN | FFL_SPACESIGN)))
			width++;
		count += ostream_writepad(s,pad - width,flags);
	}
	/* print '+' or ' ' instead of '-' */
	PRINT_SIGNED_PREFIX(count,s,n,flags);
	/* print number */
	count += s->writen(s,n);
	/* pad right */
	if((flags & FFL_PADRIGHT) && pad > 0)
		count += ostream_writepad(s,pad - count,flags);
	return count;
}

static s32 ostream_writelpad(sOStream *s,s64 n,u8 pad,u16 flags) {
	s32 count = 0;
	/* pad left */
	if(!(flags & FFL_PADRIGHT) && pad > 0) {
		u32 width = getlwidth(n);
		if(n > 0 && (flags & (FFL_FORCESIGN | FFL_SPACESIGN)))
			width++;
		count += ostream_writepad(s,pad - width,flags);
	}
	/* print '+' or ' ' instead of '-' */
	PRINT_SIGNED_PREFIX(count,s,n,flags);
	/* print number */
	count += s->writel(s,n);
	/* pad right */
	if((flags & FFL_PADRIGHT) && pad > 0)
		count += ostream_writepad(s,pad - count,flags);
	return count;
}

static s32 ostream_writeupad(sOStream *s,u32 u,u8 base,u8 pad,u16 flags) {
	s32 count = 0;
	/* pad left - spaces */
	if(!(flags & FFL_PADRIGHT) && !(flags & FFL_PADZEROS) && pad > 0) {
		u32 width = getuwidth(u,base);
		count += ostream_writepad(s,pad - width,flags);
	}
	/* print base-prefix */
	PRINT_UNSIGNED_PREFIX(count,s,base,flags);
	/* pad left - zeros */
	if(!(flags & FFL_PADRIGHT) && (flags & FFL_PADZEROS) && pad > 0) {
		u32 width = getuwidth(u,base);
		count += ostream_writepad(s,pad - width,flags);
	}
	/* print number */
	if(flags & FFL_CAPHEX)
		count += ostream_writeuchars(s,u,base,hexCharsBig);
	else
		count += ostream_writeuchars(s,u,base,hexCharsSmall);
	/* pad right */
	if((flags & FFL_PADRIGHT) && pad > 0)
		count += ostream_writepad(s,pad - count,flags);
	return count;
}

static s32 ostream_writeulpad(sOStream *s,u64 u,u8 base,u8 pad,u16 flags) {
	s32 count = 0;
	/* pad left - spaces */
	if(!(flags & FFL_PADRIGHT) && !(flags & FFL_PADZEROS) && pad > 0) {
		u32 width = getulwidth(u,base);
		count += ostream_writepad(s,pad - width,flags);
	}
	/* print base-prefix */
	PRINT_UNSIGNED_PREFIX(count,s,base,flags);
	/* pad left - zeros */
	if(!(flags & FFL_PADRIGHT) && (flags & FFL_PADZEROS) && pad > 0) {
		u32 width = getulwidth(u,base);
		count += ostream_writepad(s,pad - width,flags);
	}
	/* print number */
	if(flags & FFL_CAPHEX)
		count += ostream_writeulchars(s,u,base,hexCharsBig);
	else
		count += ostream_writeulchars(s,u,base,hexCharsSmall);
	/* pad right */
	if((flags & FFL_PADRIGHT) && pad > 0)
		count += ostream_writepad(s,pad - count,flags);
	return count;
}

s32 ostream_writedblpad(sOStream *s,double d,u8 pad,u16 flags,u32 precision) {
	s32 count = 0;
	s64 pre = (s64)d;
	/* pad left */
	if(!(flags & FFL_PADRIGHT) && pad > 0) {
		u32 width = getlwidth(pre) + precision + 1;
		if(pre > 0 && (flags & (FFL_FORCESIGN | FFL_SPACESIGN)))
			width++;
		count += ostream_writepad(s,pad - width,flags);
	}
	/* print '+' or ' ' instead of '-' */
	PRINT_SIGNED_PREFIX(count,s,pre,flags);
	/* print number */
	count += s->writedbl(s,d,precision);
	/* pad right */
	if((flags & FFL_PADRIGHT) && pad > 0)
		count += ostream_writepad(s,pad - count,flags);
	return count;
}

static s32 ostream_writepad(sOStream *s,s32 count,u16 flags) {
	s32 x = 0;
	char c = flags & FFL_PADZEROS ? '0' : ' ';
	while(count-- > 0) {
		s->writec(s,c);
		x++;
	}
	return x;
}
