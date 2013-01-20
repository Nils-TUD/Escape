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

#include <esc/common.h>
#include <esc/debug.h>
#include <esc/io.h>
#include <esc/dir.h>
#include <esc/width.h>
#include <esc/thread.h>
#include <esc/proc.h>
#include <esc/time.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define PROCINFO_BUF_SIZE	256
#define DBG_FFL_PADZEROS	1

static void debugPad(size_t count,uint flags);
static void debugStringn(char *s,uint precision);

static uint64_t start;

void printStackTrace(void) {
	uintptr_t *trace = getStackTrace();
	const char *name = getProcName();
	debugf("Process %s - stack-trace:\n",name ? name : "???");
	/* TODO maybe we should skip printStackTrace here? */
	while(*trace != 0) {
		debugf("\t0x%08x\n",*trace);
		trace++;
	}
}

const char *getProcName(void) {
	static char name[MAX_NAME_LEN];
	char buffer[PROCINFO_BUF_SIZE];
	char path[MAX_PATH_LEN];
	int fd;
	snprintf(path,sizeof(path),"/system/processes/%d/info",getpid());
	fd = open(path,IO_READ);
	if(fd >= 0) {
		if(IGNSIGS(read(fd,buffer,PROCINFO_BUF_SIZE - 1)) < 0) {
			close(fd);
			return NULL;
		}
		buffer[PROCINFO_BUF_SIZE - 1] = '\0';
		sscanf(
			buffer,
			"%*s%*hu" "%*s%*hu" "%*s%s",
			name
		);
		close(fd);
		return name;
	}
	return NULL;
}

void dbg_startUTimer(void) {
	start = getcycles();
}

void dbg_stopUTimer(const char *prefix) {
	uLongLong diff;
	diff.val64 = getcycles() - start;
	debugf("%s: 0x%08x%08x\n",prefix,diff.val32.upper,diff.val32.lower);
}

void dbg_startTimer(void) {
	start = rdtsc();
}

void dbg_stopTimer(const char *prefix) {
	uLongLong diff;
	diff.val64 = rdtsc() - start;
	debugf("%s: 0x%08x%08x\n",prefix,diff.val32.upper,diff.val32.lower);
}

void dumpBytes(const void *addr,size_t byteCount) {
	size_t i = 0;
	uchar *ptr = (uchar*)addr;
	for(i = 0; byteCount-- > 0; i++) {
		if(i % 17 == 0)
			printf("\n%08x: ",ptr);
		printf("%02x ",*ptr);
		ptr++;
	}
}

void dumpDwords(const void *addr,size_t dwordCount) {
	size_t i = 0;
	uint *ptr = (uint*)addr;
	for(i = 0; dwordCount-- > 0; i++) {
		if(i % 4 == 0)
			printf("\n%08x: ",ptr);
		printf("%08x ",*ptr);
		ptr++;
	}
}

void dumpArray(const void *array,size_t num,size_t elsize) {
	size_t i;
	ssize_t j;
	char *a = (char*)array;
	for(i = 0; i < num; i++) {
		printf("%d: ",i);
		/* little-endian... */
		for(j = elsize - 1; j >= 0; j--)
			printf("%02x",a[i * elsize + j]);
		printf("\n");
	}
}

void debugBytes(const void *addr,size_t byteCount) {
	size_t i = 0;
	uchar *ptr = (uchar*)addr;
	for(i = 0; byteCount-- > 0; i++) {
		if(i % 17 == 0)
			debugf("\n%08x: ",i);
		debugf("%02x ",*ptr);
		ptr++;
	}
}

void debugDwords(const void *addr,size_t dwordCount) {
	size_t i = 0;
	uint *ptr = (uint*)addr;
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
	uint pad,flags,precision;
	bool readFlags;
	int n;
	ulong u,u2;
	char *s;
	ullong l;
	size_t size;

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
			if(pad > 0) {
				size_t width = getnwidth(n);
				debugPad(pad - width,flags);
			}
			debugInt(n);
		}
		else if(c == 'p') {
			size = sizeof(uintptr_t);
			u = va_arg(ap, uintptr_t);
			flags |= DBG_FFL_PADZEROS;
			/* 2 hex-digits per byte and a ':' every 2 bytes */
			pad = size * 2 + size / 2;
			while(size > 0) {
				u2 = (u >> (size * 8 - 16)) & 0xFFFF;
				if(pad > 0) {
					size_t width = getuwidth(u2,16);
					debugPad(4 - width,flags);
				}
				debugUint(u2,16);
				size -= 2;
				if(size > 0)
					debugChar(':');
			}
		}
		else if(c == 'u' || c == 'o' || c == 'x') {
			uint base = c == 'o' ? 8 : (c == 'x' ? 16 : 10);
			u = va_arg(ap, int);
			if(pad > 0) {
				size_t width = getuwidth(u,base);
				debugPad(pad - width,flags);
			}
			debugUint(u,base);
		} else if(c == 's') {
			s = va_arg(ap, char*);
			if(pad > 0) {
				size_t width = precision > 0 ? MIN(precision,strlen(s)) : strlen(s);
				debugPad(pad - width,flags);
			}
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

static void debugPad(size_t count,uint flags) {
	char c = flags & DBG_FFL_PADZEROS ? '0' : ' ';
	while(count-- > 0)
		debugChar(c);
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

static void debugStringn(char *s,uint precision) {
	while(*s) {
		debugChar(*s++);
		if(precision > 0 && --precision == 0)
			break;
	}
}
