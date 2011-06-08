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

#include <sys/common.h>
#include <sys/mem/kheap.h>
#include <sys/printf.h>
#include <esc/width.h>
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

/* the minimum space-increase-size in prf_aprintc() */
#define SPRINTF_INC_SIZE	10

static void prf_printnpad(sPrintEnv *env,llong n,uint pad,uint flags);
static void prf_printupad(sPrintEnv *env,ullong u,uint base,uint pad,uint flags);
static int prf_printpad(sPrintEnv *env,int count,uint flags);
static int prf_printu(sPrintEnv *env,ullong n,uint base,char *chars);
static int prf_printn(sPrintEnv *env,llong n);
static int prf_puts(sPrintEnv *env,const char *str);
static void prf_aprintc(char c);

static char hexCharsBig[] = "0123456789ABCDEF";
static char hexCharsSmall[] = "0123456789abcdef";
static sStringBuffer *curbuf = NULL;

#ifndef __mmix__
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
	/* terminate */
	prf_aprintc('\0');
}

static void prf_aprintc(char c) {
	if(curbuf->dynamic) {
		if(curbuf->str == NULL) {
			curbuf->size = SPRINTF_INC_SIZE;
			curbuf->str = (char*)kheap_alloc(SPRINTF_INC_SIZE * sizeof(char));
		}
		if(curbuf->len >= curbuf->size) {
			char *dup;
			curbuf->size += SPRINTF_INC_SIZE;
			dup = (char*)kheap_realloc(curbuf->str,curbuf->size * sizeof(char));
			if(!dup)
				kheap_free(curbuf->str);
			curbuf->str = dup;
		}
	}
	if(curbuf->str) {
		curbuf->str[curbuf->len] = c;
		if(c != '\0')
			curbuf->len++;
	}
}
#endif

void prf_printf(sPrintEnv *env,const char *fmt,...) {
	va_list ap;
	va_start(ap,fmt);
	prf_vprintf(env,fmt,ap);
	va_end(ap);
}

void prf_vprintf(sPrintEnv *env,const char *fmt,va_list ap) {
	char c,b;
	char *s;
	llong n;
	ullong u;
	size_t size;
	uint pad,width,base,flags;
	bool readFlags;

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
					pad = va_arg(ap, ulong);
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
			case 'S':
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
				prf_printnpad(env,n,pad,flags);
				break;

			/* pointer */
			case 'p':
				size = sizeof(uintptr_t);
				u = va_arg(ap, uintptr_t);
				flags |= FFL_PADZEROS;
				/* 2 hex-digits per byte and a ':' every 2 bytes */
				pad = size * 2 + size / 2;
				while(size > 0) {
					prf_printupad(env,(u >> (size * 8 - 16)) & 0xFFFF,16,4,flags);
					size -= 2;
					if(size > 0)
						env->print(':');
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
				b = (char)va_arg(ap, uint);
				env->print(b);
				break;

			default:
				env->print(c);
				break;
		}
	}
}

static void prf_printnpad(sPrintEnv *env,llong n,uint pad,uint flags) {
	int count = 0;
	/* pad left */
	if(!(flags & FFL_PADRIGHT) && pad > 0) {
		size_t width = getllwidth(n);
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

static void prf_printupad(sPrintEnv *env,ullong u,uint base,uint pad,uint flags) {
	int count = 0;
	/* pad left - spaces */
	if(!(flags & FFL_PADRIGHT) && !(flags & FFL_PADZEROS) && pad > 0) {
		size_t width = getullwidth(u,base);
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
		size_t width = getullwidth(u,base);
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

static int prf_printpad(sPrintEnv *env,int count,uint flags) {
	int res = count;
	char c = flags & FFL_PADZEROS ? '0' : ' ';
	while(count-- > 0)
		env->print(c);
	return res;
}

static int prf_printu(sPrintEnv *env,ullong n,uint base,char *chars) {
	int res = 0;
	if(n >= base)
		res += prf_printu(env,n / base,base,chars);
	env->print(chars[(n % base)]);
	return res + 1;
}

static int prf_printn(sPrintEnv *env,llong n) {
	int res = 0;
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

static int prf_puts(sPrintEnv *env,const char *str) {
	const char *begin = str;
	char c;
	while((c = *str)) {
		env->print(c);
		str++;
	}
	return str - begin;
}
