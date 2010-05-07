/**
 * $Id: debug.c 647 2010-05-05 17:27:58Z nasmussen $
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
#include <width.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define FFL_PADZEROS	1

static void debugPad(s32 count,u16 flags);
static void debugStringn(char *s,u32 precision);

static u64 start;

void dbg_startUTimer(void) {
	start = getCycles();
}

void dbg_stopUTimer(char *prefix) {
	uLongLong diff;
	diff.val64 = getCycles() - start;
	debugf("%s: 0x%08x%08x\n",prefix,diff.val32.upper,diff.val32.lower);
}

void dbg_startTimer(void) {
	start = cpu_rdtsc();
}

void dbg_stopTimer(char *prefix) {
	uLongLong diff;
	diff.val64 = cpu_rdtsc() - start;
	debugf("%s: 0x%08x%08x\n",prefix,diff.val32.upper,diff.val32.lower);
}

void dumpBytes(void *addr,u32 byteCount) {
	u32 i = 0;
	u8 *ptr = (u8*)addr;
	for(i = 0; byteCount-- > 0; i++) {
		if(i % 17 == 0)
			printf("\n%08x: ",ptr);
		printf("%02x ",*ptr);
		ptr++;
	}
}

void dumpDwords(void *addr,u32 dwordCount) {
	u32 i = 0;
	u32 *ptr = (u32*)addr;
	for(i = 0; dwordCount-- > 0; i++) {
		if(i % 4 == 0)
			printf("\n%08x: ",ptr);
		printf("%08x ",*ptr);
		ptr++;
	}
}

void dumpArray(void *array,u32 num,u32 elsize) {
	s32 i,j;
	char *a = (char*)array;
	for(i = 0; i < (s32)num; i++) {
		printf("%d: ",i);
		/* little-endian... */
		for(j = elsize - 1; j >= 0; j--)
			printf("%02x",a[i * elsize + j]);
		printf("\n");
	}
}

void debugBytes(void *addr,u32 byteCount) {
	u32 i = 0;
	u8 *ptr = (u8*)addr;
	for(i = 0; byteCount-- > 0; i++) {
		if(i % 17 == 0)
			debugf("\n%08x: ",i);
		debugf("%02x ",*ptr);
		ptr++;
	}
}

void debugDwords(void *addr,u32 dwordCount) {
	u32 i = 0;
	u32 *ptr = (u32*)addr;
	for(i = 0; dwordCount-- > 0; i++) {
		if(i % 4 == 0)
			debugf("\n%08x: ",ptr);
		debugf("%08x ",*ptr);
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
	u8 pad,flags;
	u32 precision;
	bool readFlags;
	s32 n;
	u32 u;
	char *s;
	u64 l;

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
			n = va_arg(ap, s32);
			if(pad > 0) {
				u32 width = getnwidth(n);
				debugPad(pad - width,flags);
			}
			debugInt(n);
		}
		else if(c == 'u' || c == 'o' || c == 'x') {
			u8 base = c == 'o' ? 8 : (c == 'x' ? 16 : 10);
			u = va_arg(ap, s32);
			if(pad > 0) {
				u32 width = getuwidth(u,base);
				debugPad(pad - width,flags);
			}
			debugUint(u,base);
		} else if(c == 's') {
			s = va_arg(ap, char*);
			if(pad > 0) {
				u32 width = precision > 0 ? MIN(precision,strlen(s)) : strlen(s);
				debugPad(pad - width,flags);
			}
			debugStringn(s,precision);
		} else if(c == 'c') {
			ch = (int) va_arg(ap, s32);
			debugChar(ch);
		} else if(c == 'b') {
			l = va_arg(ap, u32);
			debugUint(l, 2);
		} else {
			debugChar(c);
		}
	}
}

static void debugPad(s32 count,u16 flags) {
	char c = flags & FFL_PADZEROS ? '0' : ' ';
	while(count-- > 0)
		debugChar(c);
}

void debugInt(s32 n) {
	if(n < 0) {
		debugChar('-');
		n = -n;
	}
	debugUint(n,10);
}

void debugUint(u32 n,u8 base) {
	u32 a;
	if((a = n / base))
		debugUint(a,base);
	debugChar("0123456789abcdef"[(int)(n % base)]);
}

void debugString(char *s) {
	debugStringn(s,0);
}

static void debugStringn(char *s,u32 precision) {
	while(*s) {
		debugChar(*s++);
		if(precision > 0 && --precision == 0)
			break;
	}
}
