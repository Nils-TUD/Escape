/**
 * $Id: int.c 153 2011-04-06 16:46:17Z nasmussen $
 */

#include "common.h"
#include "core/arith/int.h"
#include "core/cpu.h"
#include "exception.h"

octa int_umult(octa x,octa y,octa *aux) {
	tetra u[4],v[4],w[8];
	tetra t;
	int k,j,i;

	// note that this is basically the "pencil-and-paper"-method, but with the difference that
	// we multiply and add at once and not multiply at first and add afterwards (and we take a
	// wyde at once).
	// the algorithm is described in 4.3.1M of TAOCP volume 2

	// unpack multiplier and multiplicand
	u[3] = x >> 48;
	u[2] = (x >> 32) & 0xFFFF;
	u[1] = (x >> 16) & 0xFFFF;
	u[0] = (x >>  0) & 0xFFFF;
	v[3] = y >> 48;
	v[2] = (y >> 32) & 0xFFFF;
	v[1] = (y >> 16) & 0xFFFF;
	v[0] = (y >> 0) & 0xFFFF;
	// reset lower half of product
	for(j = 0; j < 4; j++)
		w[j] = 0;
	// compute product
	for(j = 0; j < 4; j++) {
		if(v[j] == 0)
			w[j + 4] = 0;
		else {
			k = 0;
			for(i = 0; i < 4; i++) {
				t = u[i] * v[j] + w[i + j] + k;
				w[i + j] = t & 0xFFFF;
				k = t >> 16;
			}
			w[j + 4] = k;
		}
	}
	// pack product into outputs
	*aux = ((octa)w[7] << 48) + ((octa)w[6] << 32) + (w[5] << 16) + w[4];
	return ((octa)w[3] << 48) + ((octa)w[2] << 32) + (w[1] << 16) + w[0];
}

octa int_smult(octa x,octa y,octa *aux) {
	octa res = int_umult(x,y,aux);
	// if op1 is negative, substract op2 from aux. this way (without overflow), we'll get FF..FF
	if(x & MSB(64))
		*aux -= y;
	// same for op2
	if(y & MSB(64))
		*aux -= x;
	return res;
}

octa int_udiv(octa x,octa y,octa z,octa *rem) {
	tetra u[8],v[4],q[4];
	int n;
	tetra vh,vmh;
	int d;
	tetra t;
	unsigned int k;
	tetra qhat,rhat;
	tetra mask;
	int j,i;

	// check that x < z, otherwise give trivial answer
	// note: if z is zero, x is always >= z, i.e. we handle division by zero here
	if(x >= z) {
		// we can't represent the result with 64-bits. MMIX defines that the remainder is y and the
		// result x in this case
		*rem = y;
		return x;
	}
	// unpack the dividend and divisor to u and v
	u[7] = x >> 48;
	u[6] = (x >> 32) & 0xFFFF;
	u[5] = (x >> 16) & 0xFFFF;
	u[4] = (x >>  0) & 0xFFFF;
	u[3] = y >> 48;
	u[2] = (y >> 32) & 0xFFFF;
	u[1] = (y >> 16) & 0xFFFF;
	u[0] = (y >>  0) & 0xFFFF;
	v[3] = z >> 48;
	v[2] = (z >> 32) & 0xFFFF;
	v[1] = (z >> 16) & 0xFFFF;
	v[0] = (z >>  0) & 0xFFFF;
	// determine the number of significand places in the divisor
	for(n = 4; v[n - 1] == 0; n--)
		;
	// normalize the divisor
	vh = v[n - 1];
	// set d to the number of steps we have to left-shift v to set the MSB
	for(d = 0; vh < 0x8000; d++)
		vh <<= 1;
	// left-shift u by d
	k = 0;
	for(j = 0; j < n + 4; j++) {
		t = (u[j] << d) + k;
		u[j] = t & 0xFFFF;
		k = t >> 16;
	}
	// left-shift v by d
	k = 0;
	for(j = 0; j < n; j++) {
		t = (v[j] << d) + k;
		v[j] = t & 0xFFFF;
		k = t >> 16;
	}
	vh = v[n - 1];
	vmh = (n > 1 ? v[n - 2] : 0);
	// compute the quotient digits
	for(j = 3; j >= 0; j--) {
		// compute the quotient digit q[j]
		// first, find the trial quotient, qhat
		t = (u[j + n] << 16) + u[j + n - 1];
		qhat = t / vh;
		rhat = t - vh * qhat;
		if(n > 1) {
			while(qhat == 0x10000 || qhat * vmh > (rhat << 16) + u[j + n - 2]) {
				qhat--;
				rhat += vh;
				if(rhat >= 0x10000)
					break;
			}
		}
		// then subtract ((qhat * v) << j) from u
		k = 0;
		for(i = 0; i < n; i++) {
			t = u[i + j] + 0xFFFF0000 - k - qhat * v[i];
			u[i + j] = t & 0xFFFF;
			k = 0xFFFF - (t >> 16);
		}
		// if the result was negative, decrease qhat by 1
		if(u[j + n] != k) {
			qhat--;
			k = 0;
			for(i = 0; i < n; i++) {
				t = u[i + j] + v[i] + k;
				u[i + j] = t & 0xFFFF;
				k = t >> 16;
			}
		}
		// finally, store the quotient digit
		q[j] = qhat;
	}
	// unnormalize the remainder
	mask = (1 << d) - 1;
	for(j = 3; j >= n; j--)
		u[j] = 0;
	// right-shift u by d
	k = 0;
	for(; j >= 0; j--) {
		t = (k << 16) + u[j];
		u[j] = t >> d;
		k = t & mask;
	}
	// pack q and u to quotient and remainder
	*rem = ((octa)u[3] << 48) + ((octa)u[2] << 32) + (u[1] << 16) + u[0];
	return ((octa)q[3] << 48) + ((octa)q[2] << 32) + (q[1] << 16) + q[0];
}

octa int_sdiv(octa x,octa y,octa *rem) {
	if(y == 0) {
		*rem = x;
		return 0;
	}
	else {
		int sx,sy;
		octa xx,yy;
		octa q,r;

		// first test and store which of x and y are negative
		// if they are make them positive for division
		if((x & MSB(64)) != 0) {
			sx = 2;
			xx = -x;
		}
		else {
			sx = 0;
			xx = x;
		}
		if((y & MSB(64)) != 0) {
			sy = 1;
			yy = -y;
		}
		else {
			sy = 0;
			yy = y;
		}

		// now divide
		q = int_udiv(0,xx,yy,&r);
		// note that the behaviour differs from x86, because MMIX uses "floored division", C99 with
		// x86 uses "truncated division". see doc/divmodnote.pdf
		switch(sx + sy) {
			// +/+
			case 0:
				break;
			// +/-
			case 1:
				// example1: 10/-3: q = 3,r = 1 => q = -4, r = -2 (10 - -2 => 12 % 3 = 0)
				// example2: 10/-2: q = 5,r = 0 => q = -5, r = 0
				if(r != 0)
					r = r - yy;
				// negate q, round towards negative infinity when there is a remainder
				q = (r != 0) ? -1 - q : -q;
				break;
			// -/+
			case 2:
				// example1: -10/3: q = 3,r = 1 => q = -4, r = 2 (-10 - 2 => -12 % 3 = 0)
				// example2: -10/2: q = 5,r = 0 => q = -5, r = 0
				if(r != 0)
					r = yy - r;
				// negate q, round towards negative infinity when there is a remainder
				q = (r != 0) ? -1 - q : -q;
				break;
			// -/-
			case 3:
				// example1: -10/-3: q = 3,r = 1 => q = 3, rem = -1 (-10 - -1 => -9 % 3 = 0)
				// example2: -2^63/-1: q = -2^63,r = 0 => overflow
				r = -r;
				break;
		}

		*rem = r;
		return q;
	}
}

octa int_shiftLeft(octa y,octa z) {
	// on x86 a shift-left of y by z means: y << (z % intwidth)
	// on mmix, it does not. so, y << 128 for example would be 0 on mmix but y on x86
	octa res;
	if(z > 63)
		res = 0;
	else
		res = y << z;
	return res;
}

octa int_shiftRightArith(octa y,octa z) {
	octa res;
	if(z > 63)
		res = (y & MSB(64)) ? -1 : 0;
	else
		res = (socta)y >> z;
	return res;
}

octa int_shiftRightLog(octa y,octa z) {
	octa res;
	if(z > 63)
		res = 0;
	else
		res = y >> z;
	return res;
}
