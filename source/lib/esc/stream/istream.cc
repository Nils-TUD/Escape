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

#include <esc/stream/istream.h>
#include <sys/esccodes.h>
#include <sys/width.h>
#include <string.h>

namespace esc {

static int getDigitVal(char c) {
	if(isdigit(c))
		return c - '0';
	return -1;
}

int IStream::getesc(int &n1,int &n2,int &n3) {
	int cmd = ESCC_INVALID;
	size_t i;
	char ec,escape[MAX_ESCC_LENGTH] = {0};
	const char *escPtr = (const char*)escape;
	for(i = 0; i < MAX_ESCC_LENGTH - 1 && (ec = read()) != ']'; i++)
		escape[i] = ec;
	if(i < MAX_ESCC_LENGTH - 1 && ec == ']')
		escape[i] = ec;
	if(good()) {
		int ln1,ln2,ln3;
		cmd = escc_get(&escPtr,&ln1,&ln2,&ln3);
		n1 = ln1;
		n2 = ln2;
		n3 = ln3;
	}
	return cmd;
}

IStream &IStream::readptr(uintptr_t &ptr) {
	char c;
	int shift = sizeof(uintptr_t) * 8 - 16;
	ptr = 0;
	while(good()) {
		uintptr_t val;
		read_unsigned(val,16);
		ptr += val << shift;
		if((c = read()) != ':') {
			putback(c);
			break;
		}
		shift -= 16;
	}
	return *this;
}

IStream &IStream::readf(double &f) {
	bool neg = false;
	int cnt = 0;
	f = 0;
	ignore_ws();

	/* handle +/- */
	char c = read();
	if(c == '-' || c == '+') {
		neg = c == '-';
		c = read();
		cnt++;
	}

	/* in front of "." */
	while(good()) {
		int val = getDigitVal(c);
		if(val == -1)
			break;
		f = f * 10 + val;
		c = read();
		cnt++;
	}

	/* after "." */
	if(c == '.') {
		cnt++;
		uint mul = 10;
		while(good()) {
			c = read();
			int val = getDigitVal(c);
			if(val == -1)
				break;
			f += (double)val / mul;
			mul *= 10;
		}
	}

	/* handle exponent */
	if(c == 'e' || c == 'E') {
		c = read();
		cnt++;
		bool negExp = c == '-';
		if(c == '-' || c == '+')
			c = read();

		int expo = 0;
		while(good()) {
			int val = getDigitVal(c);
			if(val == -1)
				break;
			expo = expo * 10 + val;
			c = read();
		}

		while(expo-- > 0)
			f *= negExp ? 0.1f : 10.f;
	}

	if(cnt > 0)
		putback(c);
	if(neg)
		f = -f;
	return *this;
}

}
