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

#include <common.h>
#include <mem/kheap.h>
#include <stdarg.h>
#include <string.h>
#include <width.h>
#include <printf.h>

/* format flags */
#define FFL_PADRIGHT		1
#define FFL_FORCESIGN		2
#define FFL_SPACESIGN		4
#define FFL_PRINTBASE		8
#define FFL_PADZEROS		16
#define FFL_CAPHEX			32

/* the minimum space-increase-size in prf_aprintc() */
#define SPRINTF_MIN_INC_SIZE	10

static void prf_printnpad(sPrintEnv *env,s32 n,u8 pad,u16 flags);
static void prf_printupad(sPrintEnv *env,u32 u,u8 base,u8 pad,u16 flags);
static s32 prf_printpad(sPrintEnv *env,s32 count,u16 flags);
static s32 prf_printu(sPrintEnv *env,u32 n,u8 base,char *chars);
static s32 prf_printn(sPrintEnv *env,s32 n);
static s32 prf_puts(sPrintEnv *env,const char *str);
static void prf_aprintc(char c);

static char hexCharsBig[] = "0123456789ABCDEF";
static char hexCharsSmall[] = "0123456789abcdef";
static sStringBuffer *curbuf = NULL;

void prf_sprintf(sStringBuffer *buf,const char *fmt,...) {
	va_list ap;
	va_start(ap,fmt);
	prf_vsprintf(buf,fmt,ap);
	va_end(ap);
}

void prf_vsprintf(sStringBuffer *buf,const char *fmt,va_list ap) {
	sPrintEnv env;
	env.print = prf_aprintc;
	env.escape = NULL;
	env.pipePad = NULL;
	curbuf = buf;
	prf_vprintf(&env,fmt,ap);
}

static void prf_aprintc(char c) {
	if(curbuf->dynamic) {
		if(curbuf->str == NULL) {
			curbuf->size = SPRINTF_MIN_INC_SIZE;
			curbuf->str = (char*)kheap_alloc(SPRINTF_MIN_INC_SIZE * sizeof(char));
		}
		if(curbuf->len >= curbuf->size) {
			curbuf->size += SPRINTF_MIN_INC_SIZE;
			curbuf->str = (char*)kheap_realloc(curbuf->str,curbuf->size * sizeof(char));
		}
	}
	if(curbuf->str)
		curbuf->str[curbuf->len++] = c;
}

void prf_printf(sPrintEnv *env,const char *fmt,...) {
	va_list ap;
	va_start(ap,fmt);
	prf_vprintf(env,fmt,ap);
	va_end(ap);
}

void prf_vprintf(sPrintEnv *env,const char *fmt,va_list ap) {
	char c,b;
	char *s;
	u8 pad;
	s32 n;
	u32 u;
	u8 width,base;
	bool readFlags;
	u16 flags;

	while(1) {
		/* wait for a '%' */
		while((c = *fmt++) != '%') {
			/* escape */
			if(c == '\033' && env->escape) {
				env->escape(&fmt);
				continue;
			}
			/* finished? */
			if(c == '\0')
				return;
			env->print(c);
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
				case '|':
					if(env->pipePad)
						pad = env->pipePad();
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

		/* determine format */
		switch(c = *fmt++) {
			/* signed integer */
			case 'd':
			case 'i':
				n = va_arg(ap, s32);
				prf_printnpad(env,n,pad,flags);
				break;

			/* pointer */
			case 'p':
				u = va_arg(ap, u32);
				flags |= FFL_PADZEROS;
				pad = 9;
				prf_printupad(env,u >> 16,16,pad - 5,flags);
				env->print(':');
				prf_printupad(env,u & 0xFFFF,16,4,flags);
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
				u = va_arg(ap, u32);
				prf_printupad(env,u,base,pad,flags);
				break;

			/* string */
			case 's':
				s = va_arg(ap, char*);
				if(pad > 0 && !(flags & FFL_PADRIGHT)) {
					width = strlen(s);
					prf_printpad(env,pad - width,flags);
				}
				n = prf_puts(env,s);
				if(pad > 0 && (flags & FFL_PADRIGHT))
					prf_printpad(env,pad - n,flags);
				break;

			/* character */
			case 'c':
				b = (char)va_arg(ap, u32);
				env->print(b);
				break;

			default:
				env->print(c);
				break;
		}
	}
}

static void prf_printnpad(sPrintEnv *env,s32 n,u8 pad,u16 flags) {
	s32 count = 0;
	/* pad left */
	if(!(flags & FFL_PADRIGHT) && pad > 0) {
		u32 width = getnwidth(n);
		if(n > 0 && (flags & (FFL_FORCESIGN | FFL_SPACESIGN)))
			width++;
		count += prf_printpad(env,pad - width,flags);
	}
	/* print '+' or ' ' instead of '-' */
	if(n > 0) {
		if((flags & FFL_FORCESIGN)) {
			env->print('+');
			count++;
		}
		else if(((flags) & FFL_SPACESIGN)) {
			env->print(' ');
			count++;
		}
	}
	/* print number */
	count += prf_printn(env,n);
	/* pad right */
	if((flags & FFL_PADRIGHT) && pad > 0)
		prf_printpad(env,pad - count,flags);
}

static void prf_printupad(sPrintEnv *env,u32 u,u8 base,u8 pad,u16 flags) {
	s32 count = 0;
	/* pad left - spaces */
	if(!(flags & FFL_PADRIGHT) && !(flags & FFL_PADZEROS) && pad > 0) {
		u32 width = getuwidth(u,base);
		count += prf_printpad(env,pad - width,flags);
	}
	/* print base-prefix */
	if((flags & FFL_PRINTBASE)) {
		if(base == 16 || base == 8) {
			env->print('0');
			count++;
		}
		if(base == 16) {
			char c = (flags & FFL_CAPHEX) ? 'X' : 'x';
			env->print(c);
			count++;
		}
	}
	/* pad left - zeros */
	if(!(flags & FFL_PADRIGHT) && (flags & FFL_PADZEROS) && pad > 0) {
		u32 width = getuwidth(u,base);
		count += prf_printpad(env,pad - width,flags);
	}
	/* print number */
	if(flags & FFL_CAPHEX)
		count += prf_printu(env,u,base,hexCharsBig);
	else
		count += prf_printu(env,u,base,hexCharsSmall);
	/* pad right */
	if((flags & FFL_PADRIGHT) && pad > 0)
		prf_printpad(env,pad - count,flags);
}

static s32 prf_printpad(sPrintEnv *env,s32 count,u16 flags) {
	s32 res = count;
	char c = flags & FFL_PADZEROS ? '0' : ' ';
	while(count-- > 0)
		env->print(c);
	return res;
}

static s32 prf_printu(sPrintEnv *env,u32 n,u8 base,char *chars) {
	s32 res = 0;
	if(n >= base)
		res += prf_printu(env,n / base,base,chars);
	env->print(chars[(n % base)]);
	return res + 1;
}

static s32 prf_printn(sPrintEnv *env,s32 n) {
	s32 res = 0;
	if(n < 0) {
		env->print('-');
		n = -n;
		res++;
	}

	if(n >= 10)
		res += prf_printn(env,n / 10);
	env->print('0' + n % 10);
	return res + 1;
}

static s32 prf_puts(sPrintEnv *env,const char *str) {
	const char *begin = str;
	char c;
	while((c = *str)) {
		env->print(c);
		str++;
	}
	return str - begin;
}
