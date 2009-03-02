/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/debug.h"
#include <stdarg.h>

/**
 * Prints the given char
 *
 * @param c the character
 */
extern void debugChar(s8 c);

void debugf(cstring fmt,...) {
	va_list ap;
	va_start(ap, fmt);
	vdebugf(fmt,ap);
	va_end(ap);
}

void vdebugf(cstring fmt,va_list ap) {
	s8 c,ch;
	s32 n;
	u32 u;
	string s;
	u64 l;

	while(1) {
		while((c = *fmt++) != '%') {
			if(c == '\0') {
				return;
			}
			debugChar(c);
		}

		/* color given? */
		if(*fmt == ':') {
			/* TODO ignore color since it is not supported yet */
			fmt += 3;
		}

		c = *fmt++;
		if(c == 'd') {
			n = va_arg(ap, s32);
			debugInt(n);
		}
		else if(c == 'u' || c == 'o' || c == 'x') {
			u = va_arg(ap, s32);
			debugUint(u, c == 'o' ? 8 : (c == 'x' ? 16 : 10));
		} else if(c == 's') {
			s = va_arg(ap, string);
			debugString(s);
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
	debugChar("0123456789ABCDEF"[(int)(n % base)]);
}

void debugString(string s) {
	while(*s) {
		debugChar(*s++);
	}
}
