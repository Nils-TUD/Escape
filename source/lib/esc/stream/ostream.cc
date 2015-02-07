/*
 * Copyright (C) 2013, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of M3 (Microkernel for Minimalist Manycores).
 *
 * M3 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * M3 is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#include <esc/stream/ostream.h>
#include <algorithm>
#include <string.h>
#include <math.h>

namespace esc {

char OStream::_hexchars_big[]     = "0123456789ABCDEF";
char OStream::_hexchars_small[]   = "0123456789abcdef";

int OStream::printsignedprefix(long n,uint flags) {
	int count = 0;
	if(n > 0) {
		if(flags & FORCESIGN) {
			write('+');
			count++;
		}
		else if(flags & SPACESIGN) {
			write(' ');
			count++;
		}
	}
	return count;
}

int OStream::putspad(const char *s,uint pad,uint prec,uint flags) {
	int count = 0;
	if(pad > 0 && !(flags & PADRIGHT)) {
		ulong width = prec != static_cast<uint>(-1) ? std::min<size_t>(prec,strlen(s)) : strlen(s);
		count += printpad(pad - width,flags);
	}
	count += puts(s,prec);
	if(pad > 0 && (flags & PADRIGHT))
		count += printpad(pad - count,flags);
	return count;
}

int OStream::printnpad(llong n,uint pad,uint flags) {
	int count = 0;
	// pad left
	if(!(flags & PADRIGHT) && pad > 0) {
		size_t width = std::count_digits(n,10);
		if(n > 0 && (flags & (FORCESIGN | SPACESIGN)))
			width++;
		count += printpad(pad - width,flags);
	}
	count += printsignedprefix(n,flags);
	count += printn(n);
	// pad right
	if((flags & PADRIGHT) && pad > 0)
		count += printpad(pad - count,flags);
	return count;
}

int OStream::printupad(ullong u,uint base,uint pad,uint flags) {
	int count = 0;
	// pad left - spaces
	if(!(flags & PADRIGHT) && !(flags & PADZEROS) && pad > 0) {
		size_t width = std::count_digits(u,base);
		count += printpad(pad - width,flags);
	}
	// print base-prefix
	if((flags & PRINTBASE)) {
		if(base == 16 || base == 8) {
			write('0');
			count++;
		}
		if(base == 16) {
			char c = (flags & CAPHEX) ? 'X' : 'x';
			write(c);
			count++;
		}
	}
	// pad left - zeros
	if(!(flags & PADRIGHT) && (flags & PADZEROS) && pad > 0) {
		size_t width = std::count_digits(u,base);
		count += printpad(pad - width,flags);
	}
	// print number
	if(flags & CAPHEX)
		count += printu(u,base,_hexchars_big);
	else
		count += printu(u,base,_hexchars_small);
	// pad right
	if((flags & PADRIGHT) && pad > 0)
		count += printpad(pad - count,flags);
	return count;
}

int OStream::printpad(int count,uint flags) {
	int res = count;
	char c = flags & PADZEROS ? '0' : ' ';
	while(count-- > 0)
		write(c);
	return res;
}

int OStream::printu(ullong n,uint base,char *chars) {
	int res = 0;
	if(n >= base)
		res += printu(n / base,base,chars);
	write(chars[n % base]);
	return res + 1;
}

int OStream::printn(llong n) {
	int res = 0;
	if(n < 0) {
		write('-');
		n = -n;
		res++;
	}

	if(n >= 10)
		res += printn(n / 10);
	write('0' + (n % 10));
	return res + 1;
}

int OStream::printdbl(double d,uint precision,uint pad,int flags) {
	int c = 0;
	llong pre = (llong)d;

	// pad left
	if(!(flags & PADRIGHT) && pad > 0) {
		size_t width;
		if(isnan(d) || isinf(d))
			width = (d < 0 || (flags & (FORCESIGN | SPACESIGN))) ? 4 : 3;
		else
			width = std::count_digits(pre,10) + precision + 1;
		if(pre > 0 && (flags & (FORCESIGN | SPACESIGN)))
			width++;
		if(pad > width)
			c += printpad(pad - width,flags);
	}

	bool nan = isnan(d);
	bool inf = isinf(d);
	if(d < 0) {
		d = -d;
		if(nan || inf) {
			write('-');
			c++;
		}
	}

	if(nan)
		c += puts("nan",-1);
	else if(inf)
		c += puts("inf",-1);
	else {
		c += printn(pre);
		d -= pre;
		write('.');
		c++;
		while(precision-- > 0) {
			d *= 10;
			pre = (long)d;
			write((pre % 10) + '0');
			d -= pre;
			c++;
		}
	}

	// pad right
	if((flags & PADRIGHT) && (int)pad > c)
		c += printpad(pad - c,flags);
	return c;
}

int OStream::printptr(uintptr_t u,uint flags) {
	int count = 0;
	size_t size = sizeof(uintptr_t);
	flags |= PADZEROS;
	// 2 hex-digits per byte and a ':' every 2 bytes
	while(size > 0) {
		count += printupad((u >> (size * 8 - 16)) & 0xFFFF,16,4,flags);
		size -= 2;
		if(size > 0) {
			write(':');
			// don't print the base again
			flags &= ~PRINTBASE;
			count++;
		}
	}
	return count;
}

int OStream::puts(const char *str,ulong prec) {
	const char *begin = str;
	char c;
	while((prec == static_cast<ulong>(-1) || prec-- > 0) && (c = *str)) {
		write(c);
		str++;
	}
	return str - begin;
}

}
