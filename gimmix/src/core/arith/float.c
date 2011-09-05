/**
 * $Id: float.c 217 2011-06-03 18:11:57Z nasmussen $
 */

#include <assert.h>

#include "common.h"
#include "core/arith/float.h"
#include "core/arith/int.h"
#include "core/cpu.h"
#include "core/register.h"
#include "mmix/io.h"
#include "exception.h"

// Note that nearly all comments here are written by myself. Due to the difficult subject,
// please question everything you see here. Although I've done my best, I guess not everything
// is 100 percent correct ;)

#define ZERO_EXPONENT	(-1000)		// zero is assumed to have this exponent

static sFloat fl_unpack(octa x);
static octa fl_pack(sFloat fl,int rounding,unsigned *ex);
static sFloat fl_sunpack(tetra x);
static tetra fl_spack(sFloat fl,int rounding,unsigned *ex);
static octa fl_remNum(sFloat fy,sFloat fz,int delta,unsigned *ex);
static octa fl_sqrtNum(sFloat fz,int rounding,unsigned *ex);
//static bool fl_isNear(octa y,octa z,sFloat fz,sFloat fe);
static octa fl_addNums(sFloat fy,sFloat fz,unsigned *ex);
static int fl_getRoundMode(void);
static octa fl_handleNaN(octa y,octa z,eFtype yt,eFtype zt,unsigned *ex);
static octa fl_round(octa x,int sign,int rounding);

static const octa infinity = 0x7FF0000000000000;
static const octa stdNaN = 0x7FF8000000000000;

static sFloat fl_unpack(octa x) {
	int ee;
	sFloat fl;
	fl.s = (x & MSB(64)) ? '-' : '+';
	fl.f = x << 2;
	fl.f &= 0x0003FFFFFFFFFFFFF;
	ee = (x >> 52) & 0x7FF;
	// normalized float
	if(ee) {
		fl.e = ee - 1;
		fl.f |= 0x40000000000000;
		if(ee < 0x7FF)
			fl.type = NUM;
		// exponent is 1...1; if fraction is zero, its infinitive
		else if(fl.f == 0x40000000000000)
			fl.type = INF;
		// otherwise its NaN
		else
			fl.type = NAN;
	}
	// unnormalized float && fraction is zero
	else if(fl.f == 0) {
		fl.e = ZERO_EXPONENT;
		fl.type = ZERO;
	}
	// unnormalized float
	else {
		// normalize it. i.e. we shift the fraction left and decrement the exponent until the
		// leftmost bit of the fraction is set
		do {
			ee--;
			fl.f <<= 1;
		}
		while(!(fl.f & 0x40000000000000));
		fl.e = ee;
		fl.type = NUM;
	}
	return fl;
}

static octa fl_pack(sFloat fl,int rounding,unsigned *ex) {
	octa o;
	// exponent 1...1
	if(fl.e > 0x7FD) {
		fl.e = 0x7FF;
		o = 0;
	}
	else {
		// negative e means unnormalized
		if(fl.e < 0) {
			// larger than precision? than the shift is 0 anyway and +1 because of the sticky-bit
			if(fl.e < -54)
				o = 1;
			else {
				// divide by exponent and multiply again
				octa oo;
				o = fl.f >> -fl.e;
				oo = o << -fl.e;
				// if the result is different, its inexact
				if(oo != fl.f)
					o |= 1;		// sticky bit
			}
			// keep it unnormalized
			fl.e = 0;
		}
		else
			o = fl.f;
	}

	// we use the last 2 bits for inexact-detection
	if(o & 3)
		*ex |= TRIP_FLOATINEXACT;
	o = fl_round(o,fl.s,rounding);

	// shift fraction back (see unpack) and add exponent
	o >>= 2;
	// note: we add 1 to e by the addition because the fraction has bit 2^54 set
	o += (octa)fl.e << 52;

	// overflow?
	if(o >= 0x7FF0000000000000)
		*ex |= TRIP_FLOATOVER + TRIP_FLOATINEXACT;
	// underflow?
	else if(o < 0x0010000000000000)
		*ex |= TRIP_FLOATUNDER;

	if(fl.s == '-')
		o |= MSB(64);
	return o;
}

static sFloat fl_sunpack(tetra x) {
	int ee;
	sFloat fl;
	fl.s = (x & MSB(32)) ? '-' : '+';
	// the first 22 bit fraction are put into the upper half of fl.f
	// the LSB is put into the MSB of the lower part
	fl.f = (octa)(x & 0x7FFFFF) << 31;
	ee = (x >> 23) & 0xFF;
	if(ee) {
		// 0x380 + 127 (bias of 32-bit floats) = 1023 (bias of 64-bit floats)
		fl.e = ee + 0x380 - 1;
		fl.f |= 0x40000000000000;
		if(ee < 0xFF)
			fl.type = NUM;
		// exponent is 1...1; if fraction is zero, its infinitive
		else if((x & 0x7FFFFFFF) == 0x7F800000)
			fl.type = INF;
		else
			fl.type = NAN;
	}
	// unnormalized float && fraction is zero
	else if(!(x & 0x7FFFFFFF)) {
		fl.e = ZERO_EXPONENT;
		fl.type = ZERO;
	}
	// unnormalized float
	else {
		do {
			ee--;
			fl.f <<= 1;
		}
		while(!(fl.f & 0x40000000000000));
		fl.e = ee + 0x380;
		fl.type = NUM;
	}
	return fl;
}

static tetra fl_spack(sFloat fl,int rounding,unsigned *ex) {
	tetra o;
	// 0x47D - 0x380 = 0xFD; -> exponent = 1...1
	if(fl.e > 0x47D) {
		fl.e = 0x47F;
		o = 0;
	}
	else {
		// we want to keep 2 bits behind the 23-bit fraction
		o = fl.f >> 29;
		// if lower bits are set in the fraction, its inexact anyway
		if(fl.f & 0x1FFFFFFF)
			o |= 1;
		// negative e means unnormalized
		if(fl.e < 0x380) {
			// larger than short-precision?
			if(fl.e < 0x380 - 25)
				o = 1;
			else {
				// divide by exponent and multiply again
				tetra oO,oo;
				oO = o;
				o = o >> (0x380 - fl.e);
				oo = o << (0x380 - fl.e);
				// if the result is different, its inexact
				if(oo != oO)
					o |= 1;
			}
			// keep it unnormalized
			fl.e = 0x380;
		}
	}

	// we use the last 2 bits for inexact-detection
	if(o & 3)
		*ex |= TRIP_FLOATINEXACT;
	o = fl_round(o,fl.s,rounding);

	// shift fraction back (see unpack) and add exponent
	o >>= 2;
	// note: we add 1 to e by the addition because the fraction has bit 2^25 set
	o += (fl.e - 0x380) << 23;

	// overflow?
	if(o >= 0x7F800000)
		*ex |= TRIP_FLOATOVER + TRIP_FLOATINEXACT;
	// underflow?
	else if(o < 0x00100000)
		*ex |= TRIP_FLOATUNDER;

	if(fl.s == '-')
		o |= MSB(32);
	return o;
}

octa fl_shortToFloat(tetra z,unsigned *ex) {
	sFloat fz = fl_sunpack(z);
	octa x;
	switch(fz.type) {
		case ZERO:
			x = 0;
			break;
		case NUM:
			return fl_pack(fz,ROUND_OFF,ex);
		case INF:
			x = infinity;
			break;
		case NAN:
			x = fz.f >> 2;
			x |= 0x7FF0000000000000;
			break;
	}
	if(fz.s == '-')
		x |= MSB(64);
	return x;
}

tetra fl_floatToShort(octa x,unsigned *ex) {
	tetra z;
	sFloat fx = fl_unpack(x);
	switch(fx.type) {
		case ZERO:
			z = 0;
			break;
		case NUM:
			return fl_spack(fx,fl_getRoundMode(),ex);
		case INF:
			z = 0x7F800000;
			break;
		case NAN:
			// is NaN signaling?
			if(!(fx.f & 0x20000000000000)) {
				fx.f |= 0x20000000000000;
				*ex |= TRIP_FLOATINVOP;
			}
			// ignore 29 right-most fraction bits (+2 because our fraction is shifted left by 2)
			z = 0x7F800000 | fx.f >> 31;
			break;
	}
	if(fx.s == '-')
		z |= MSB(32);
	return z;
}

octa fl_mult(octa y,octa z,unsigned *ex) {
	sFloat fy = fl_unpack(y);
	sFloat fz = fl_unpack(z);
	// will be '-' when the result is negative
	char xs = fy.s + fz.s - '+';

	// first handle the NaN cases
	octa x = fl_handleNaN(y,z,fy.type,fz.type,ex);
	if(x != 0)
		return x;

	switch(4 * fy.type + fz.type) {
		// if one of them is ZERO, the result is ZERO
		case 4 * NUM + ZERO:
		case 4 * ZERO + NUM:
		case 4 * ZERO + ZERO:
			x = 0;
			break;
		// if one of them is INF, the result is INF as well
		case 4 * NUM + INF:
		case 4 * INF + NUM:
		case 4 * INF + INF:
			x = infinity;
			break;
		// ZERO * INF results in NAN
		case 4 * ZERO + INF:
		case 4 * INF + ZERO:
			x = stdNaN;
			*ex |= TRIP_FLOATINVOP;
			break;

		// default case
		case 4 * NUM + NUM: {
			sFloat fx;
			octa aux;
			fx.type = NUM;
			fx.s = xs;
			// 2^x * 2^y = 2^(x+y)
			fx.e = fy.e + fz.e - 0x3FD;
			// without shift, the result of this multiplication is spread over aux and x
			// to be able to ignore x, we shift fz.f left by 9
			x = int_umult(fy.f,fz.f << 9,&aux);
			if(aux >= 0x40000000000000)
				fx.f = aux;
			else {
				// normalize
				fx.f = aux << 1;
				fx.e--;
			}
			// adjust the sticky-bit (bits in x set means: inexact)
			if(x)
				fx.f |= 1;
			return fl_pack(fx,fl_getRoundMode(),ex);
		}
	}
	if(xs == '-')
		x |= MSB(64);
	return x;
}

octa fl_divide(octa y,octa z,unsigned *ex) {
	sFloat fy = fl_unpack(y);
	sFloat fz = fl_unpack(z);
	// will be '-' when the result is negative
	char xs = fy.s + fz.s - '+';

	// first handle the NaN cases
	octa x = fl_handleNaN(y,z,fy.type,fz.type,ex);
	if(x != 0)
		return x;

	switch(4 * fy.type + fz.type) {
		// 0 / X = 0 and X / INF = 0
		case 4 * ZERO + INF:
		case 4 * ZERO + NUM:
		case 4 * NUM + INF:
			x = 0;
			break;
		// X / 0 -> div by zero
		case 4 * NUM + ZERO:
			*ex |= TRIP_FLOATDIVBY0;
			// fall through
		// INF / X = INF
		case 4 * INF + NUM:
		case 4 * INF + ZERO:
			x = infinity;
			break;
		// 0 / 0 = NaN and INF / INF = NaN
		case 4 * ZERO + ZERO:
		case 4 * INF + INF:
			x = stdNaN;
			*ex |= TRIP_FLOATINVOP;
			break;
		// default case
		case 4 * NUM + NUM: {
			sFloat fx;
			octa rem;
			fx.type = NUM;
			fx.s = xs;
			// 2^x / 2^y = 2^(x-y)
			fx.e = fy.e - fz.e + 0x3FD;
			// same trick as above: shift left by 9 to be able to use fx.f for fraction
			fx.f = int_udiv(fy.f,0,fz.f << 9,&rem);
			if(fx.f >= 0x80000000000000) {
				rem |= fx.f & 1;
				// normalize
				fx.f >>= 1;
				fx.e++;
			}
			// adjust the sticky-bit (bits in rem set means: inexact)
			if(rem)
				fx.f |= 1;
			return fl_pack(fx,fl_getRoundMode(),ex);
		}
	}
	if(xs == '-')
		x |= MSB(64);
	return x;
}

octa fl_add(octa y,octa z,unsigned *ex) {
	sFloat fy = fl_unpack(y);
	sFloat fz = fl_unpack(z);
	char xs = '+';	// prevent uninitialized-warning

	// first handle the NaN cases
	octa x = fl_handleNaN(y,z,fy.type,fz.type,ex);
	if(x != 0)
		return x;

	switch(4 * fy.type + fz.type) {
		case 4 * ZERO + NUM:
			return fl_pack(fz,ROUND_OFF,ex);	// may underflow
		case 4 * NUM + ZERO:
			return fl_pack(fy,ROUND_OFF,ex);	// may underflow
		case 4 * INF + INF:
			// +INF + -INF is not defined
			if(fy.s != fz.s) {
				*ex |= TRIP_FLOATINVOP;
				x = stdNaN;
			}
			else
				x = infinity;
			xs = fz.s;
			break;
		case 4 * NUM + INF:
		case 4 * ZERO + INF:
			x = infinity;
			xs = fz.s;
			break;
		case 4 * INF + NUM:
		case 4 * INF + ZERO:
			x = infinity;
			xs = fy.s;
			break;
		case 4 * NUM + NUM:
			// X + -Y = 0 and -X + Y = 0
			if(y != (z ^ 0x8000000000000000))
				return fl_addNums(fy,fz,ex);
			// fall through
		case 4 * ZERO + ZERO:
			x = 0;
			xs = (fy.s == fz.s ? fy.s : fl_getRoundMode() == ROUND_DOWN ? '-' : '+');
			break;
	}
	if(xs == '-')
		x |= MSB(64);
	return x;
}

octa fl_sub(octa y,octa z,unsigned *ex) {
	// if z is not NaN, swap the sign
	if(fl_compare(z,0) != CMP_UNORDERED)
		z ^= MSB(64);
	return fl_add(y,z,ex);
}

octa fl_compareWithEps(octa y,octa z,octa e,int similarity) {
	sFloat fe = fl_unpack(e);
	if(fe.s == '-')
		return ECMP_INVALID;
	switch(fe.type) {
		case NAN:
			return ECMP_INVALID;
		case INF:
			// any big exponent is sufficient
			fe.e = 10000;
			break;
		case NUM:
		case ZERO:
			break;
	}

	sFloat fy = fl_unpack(y);
	sFloat fz = fl_unpack(z);
	switch(4 * fy.type + fz.type) {
		// at least one NaN means, not comparable
		case 4 * NAN + NAN:
		case 4 * NAN + INF:
		case 4 * NAN + NUM:
		case 4 * NAN + ZERO:
		case 4 * INF + NAN:
		case 4 * NUM + NAN:
		case 4 * ZERO + NAN:
			return ECMP_INVALID;

		// N(+/-INF) = {everything} if e >= 2
		// -> if fe.e >= 1023, two INFs are equal
		case 4 * INF + INF:
			return (fy.s == fz.s || fe.e >= 1023) ? ECMP_EQUAL : ECMP_NOT_EQUAL;

		// those are considered equal if similarity should be compared and e >= 1
		// because: N(+/-INF) = {everything except +/-INF} if 1 <= e < 2
		case 4 * INF + NUM:
		case 4 * INF + ZERO:
		case 4 * NUM + INF:
		case 4 * ZERO + INF:
			return (similarity && fe.e >= 1022) ? ECMP_EQUAL : ECMP_NOT_EQUAL;

		// obviously, 0 and 0 are equal
		case 4 * ZERO + ZERO:
			return ECMP_EQUAL;

		// if we don't want to compare for similarity, zero and not-zero is not equal.
		// if similarity should be compared, we have to compare them in detail.
		// the reason is that knuth defines N(0) = {0}. so, zero and not-zero is never equal
		// with respect to an epsilon.
		case 4 * ZERO + NUM:
		case 4 * NUM + ZERO:
			if(!similarity)
				return ECMP_NOT_EQUAL;
			break;

		// detailed comparison
		case 4 * NUM + NUM:
			break;
	}

	// unsubnormalize y and z, if they are subnormal
	if(fy.e < 0 && fy.type != ZERO) {
		fy.f = y << 2;
		fy.e = 0;
	}
	if(fz.e < 0 && fz.type != ZERO) {
		fz.f = z << 2;
		fz.e = 0;
	}

	// we want to have: fy >= fz (ignoring sign)
	if(fy.e < fz.e || (fy.e == fz.e && fy.f < fz.f)) {
		sFloat ft = fy;
		fy = fz;
		fz = ft;
	}

	// take care of zero (has a special exponent that doesn't work here)
	if(fz.e == ZERO_EXPONENT)
		fz.e = fy.e;
	// The relation y ~~ z (e) reduces to y ~ z (e/2^d)
	int d = fy.e - fz.e;
	if(!similarity)
		fe.e -= d;

	// if e >= 2, z in N(y)
	if(fe.e >= 1023)
		return ECMP_EQUAL;

	// basically we're doing the following:
	// fy.f + (-1)^(fy.s==fz.s) * fz.f / 2^d <= 2^(fe.e - 1021) * fe.f
	// TODO unfortunatly I've not been able to figure out how knuth gets from
	// y - z <= 2^(ze-1022) * epsilon
	// to the formular above

	// Compute the difference of fraction parts, fdiff
	octa fdiff,fcheck;
	// more than our precision?
	if(d > 54) {
		fdiff = 0;
		fcheck = fz.f;
	}
	else {
		// fz.f / 2^d
		fdiff = fz.f >> d;
		fcheck = fdiff << d;
	}
	// truncated result, hence d > 2
	if(fcheck != fz.f) {
		// difference too large for similarity?
		if(fe.e < 1020)
			return ECMP_NOT_EQUAL;
		// adjust for ceiling
		fdiff += fy.s == fz.s ? 0 : 1;
	}
	// fdiff = fy.f + (-1)^(fy.s==fz.s) * fz.f / 2^d
	// note that for the case d > 54, fdiff can never be 0, because fy.f has bit 2^54 set
	fdiff = fy.s == fz.s ? fy.f - fdiff : fy.f + fdiff;

	if(fdiff == 0)
		return ECMP_EQUAL;
	// if y != z and e < 2^-54, y !~ z
	if(fe.e < 968)
		return ECMP_NOT_EQUAL;

	// fdiff <= 2^(fe.e - 1021) * fe.f
	if(fe.e >= 1021)
		fe.f <<= fe.e - 1021;
	else
		fe.f >>= 1021 - fe.e;
	return fdiff <= fe.f ? ECMP_EQUAL : ECMP_NOT_EQUAL;
}

octa fl_compare(octa y,octa z) {
	sFloat fy = fl_unpack(y);
	sFloat fz = fl_unpack(z);
	octa x;
	switch(4 * fy.type + fz.type) {
		// if NaN is involved, its always unordered
		case 4 * NAN + NAN:
		case 4 * ZERO + NAN:
		case 4 * NUM + NAN:
		case 4 * INF + NAN:
		case 4 * NAN + ZERO:
		case 4 * NAN + NUM:
		case 4 * NAN + INF:
			return CMP_UNORDERED;

		// obviously, 0 and 0 are equal
		case 4 * ZERO + ZERO:
			return CMP_EQUAL;

		// default case
		case 4 * ZERO + NUM:
		case 4 * NUM + ZERO:
		case 4 * ZERO + INF:
		case 4 * INF + ZERO:
		case 4 * NUM + NUM:
		case 4 * NUM + INF:
		case 4 * INF + NUM:
		case 4 * INF + INF:
			if(fy.s != fz.s)
				x = CMP_GREATER;
			else if(y > z)
				x = CMP_GREATER;
			else if(z > y)
				x = CMP_LESS;
			else
				return CMP_EQUAL;
			break;
	}
	// if y is negative, the result is the opposite
	return fy.s == '-' ? -x : x;
}

octa fl_interize(octa z,int rounding,unsigned *ex) {
	sFloat fz = fl_unpack(z);
	if(!rounding)
		rounding = fl_getRoundMode();
	switch(fz.type) {
		case NAN:
			if(!(z & 0x8000000000000)) {
				*ex |= TRIP_FLOATINVOP;
				z |= 0x8000000000000;
			}
			return z;

		case INF:
		case ZERO:
			return z;

		case NUM:
			break;
	}

	// already an integer? (e - 1022) >= fraction-bits, i.e. we have no fraction anyway
	if(fz.e >= 1074)
		return fl_pack(fz,ROUND_OFF,ex);

	octa xf;
	// z < 0.5? then the result is always 0
	if(fz.e <= 1020)
		xf = 1;
	else {
		// move out all fraction-bits
		octa oo;
		xf = fz.f >> (1074 - fz.e);
		oo = xf << (1074 - fz.e);
		// set sticky bit
		if(oo != fz.f)
			xf |= 1;
	}

	// round the fraction
	xf = fl_round(xf,fz.s,rounding);
	xf &= 0xFFFFFFFFFFFFFFFC;
	// z >= 1?
	if(fz.e >= 1022) {
		sFloat fx;
		// move the fraction back, corresponding to the exponent
		// this way, all fraction-bits are gone, but the exponent is kept
		fx.f = xf << (1074 - fz.e);
		fx.e = fz.e;
		fx.s = fz.s;
		return fl_pack(fx,ROUND_OFF,ex);
	}

	// round up to 1?
	if(xf & 0xFFFFFFFF)
		xf = 0x3FF0000000000000;
	// set sign
	if(fz.s == '-')
		xf |= MSB(64);
	return xf;
}

octa fl_fixit(octa z,int rounding,unsigned *ex) {
	sFloat fz = fl_unpack(z);
	if(!rounding)
		rounding = fl_getRoundMode();
	switch(fz.type) {
		case NAN:
		case INF:
			*ex |= TRIP_FLOATINVOP;
			return z;
		case ZERO:
			return 0;
		case NUM:
			break;
	}

	octa o;
	fz = fl_unpack(fl_interize(z,rounding,ex));
	if(fz.type == ZERO)
		return 0;
	// <= our precision? (1076-54 = 1022)
	if(fz.e <= 1076)
		o = fz.f >> (1076 - fz.e);
	else {
		// > 2^63-1 (1085 = 1022+63)
		if(fz.s == '+' && fz.e >= 1085)
			*ex |= TRIP_FLOAT2FIXOVER;
		// < -2^63
		else if(fz.s == '-' && (fz.e > 1085 || (fz.e == 1085 && fz.f > 0x40000000000000)))
			*ex |= TRIP_FLOAT2FIXOVER;
		// left shift by more than 63 yields 0 (1140-1076 = 64)
		if(fz.e >= 1140)
			return 0;
		o = fz.f << (fz.e - 1076);
	}
	return fz.s == '-' ? 0 - o : o;
}

octa fl_floatit(octa z,int rounding,bool unSigned,bool shortPrec,unsigned *ex) {
	sFloat x;
	if(z == 0)
		return 0;
	if(!rounding)
		rounding = fl_getRoundMode();

	x.f = z;
	// take care of sign, if signed is requested
	if(!unSigned && (x.f & MSB(64))) {
		x.s = '-';
		x.f = 0 - x.f;
	}
	else
		x.s = '+';

	// normalize it; we know that it isn't zero; start with 1076 (1022+54), because we have 54
	// precision bits
	x.e = 1076;
	while(x.f < 0x40000000000000) {
		x.e--;
		x.f <<= 1;
	}
	// alternatively, it can be bigger than 2^54; so shift it right until the fraction is ok.
	// in this case we may loose precision
	while(x.f >= 0x80000000000000) {
		int t = x.f & 1;
		x.e++;
		x.f >>= 1;
		x.f |= t;
	}

	if(shortPrec) {
		// simply pack and unpack it to round it to short-precision
		tetra t = fl_spack(x,rounding,ex);
		x = fl_sunpack(t);
	}
	return fl_pack(x,rounding,ex);
}

octa fl_sqrt(octa z,int rounding,unsigned *ex) {
	sFloat fz = fl_unpack(z);
	if(!rounding)
		rounding = fl_getRoundMode();

	octa x;
	// sqrt of negative numbers is undefined
	if(fz.s == '-' && fz.type != ZERO) {
		*ex |= TRIP_FLOATINVOP;
		x = stdNaN;
	}
	else {
		switch(fz.type) {
			case NAN:
				if(!(z & 0x8000000000000)) {
					*ex |= TRIP_FLOATINVOP;
					z |= 0x8000000000000;
				}
				return z;

			case INF:
			case ZERO:
				x = z;
				break;

			case NUM:
				return fl_sqrtNum(fz,rounding,ex);
		}
	}
	if(fz.s == '-')
		x |= MSB(64);
	return x;
}

octa fl_remstep(octa y,octa z,int delta,unsigned *ex) {
	sFloat fy = fl_unpack(y);
	sFloat fz = fl_unpack(z);

	// first handle the NaN cases
	octa x = fl_handleNaN(y,z,fy.type,fz.type,ex);
	if(x != 0)
		return x;

	switch(4 * fy.type + fz.type) {
		// X / 0 and INF / X is undefined
		case 4 * ZERO + ZERO:
		case 4 * NUM + ZERO:
		case 4 * INF + ZERO:
		case 4 * INF + NUM:
		case 4 * INF + INF:
			x = stdNaN;
			*ex |= TRIP_FLOATINVOP;
			break;

		// the remainder of 0 / X is X and X / INF = INF
		case 4 * ZERO + NUM:
		case 4 * ZERO + INF:
		case 4 * NUM + INF:
			return y;

		// the default case
		case 4 * NUM + NUM:
			x = fl_remNum(fy,fz,delta,ex);
			// inherit the original sign of y when the result is zero
			if(x != 0)
				return x;
			break;
	}
	if(fy.s == '-')
		x |= MSB(64);
	return x;
}

static octa fl_remNum(sFloat fy,sFloat fz,int delta,unsigned *ex) {
	sFloat fx;
	int odd = 0;	// will be 1 if we've substracted an odd multiple of z from y
	int thresh = fy.e - delta;
	// don't walk beyound fz.e
	if(thresh < fz.e)
		thresh = fz.e;
	// I think, the basic idea here is that we just have to calculate the remainder for the
	// fraction, because the exponents are both powers of 2. Therefore, as long as the floats
	// are normalized, we can simply substract the fractions because the exponents don't play
	// a role. We do that until the fractions are either zero (which means, remainder is zero)
	// or until fy.e <= fz.e - 1
	while(fy.e >= thresh) {
		// if the fraction is equal the remainder is always zero. because the exponents are
		// powers of 2, i.e. even if they're different, 2^x % 2^y is always zero.
		if(fy.f == fz.f)
			return 0;

		// substract (fy.e,fy.f) by a multiple of fz.f
		if(fy.f < fz.f) {
			// appropriate?
			if(fy.e == fz.e)
				goto tryComplement;
			// multiply fraction by 2, decrease exponent.
			// note also that bit 2^54 is always set, i.e. fy.f is always >= fz.f afterwards
			fy.e--;
			fy.f <<= 1;
		}
		fy.f -= fz.f;

		// TODO ?
		if(fy.e == fz.e)
			odd = 1;
		// correct exponent and fraction, so that the bit 2^54 is set again
		while(fy.f < 0x40000000000000) {
			fy.e--;
			fy.f <<= 1;
		}
	}
	// this can only happen if thresh is too low so that we need multiple steps to calculate the
	// remainder. in our case, this can't happen
	if(fy.e >= fz.e) {
		assert(false);
		return fl_pack(fy,ROUND_OFF,ex);
	}
	// if y < z/2, the result is y; TODO ??
	if(fy.e < fz.e - 1)
		return fl_pack(fy,ROUND_OFF,ex);

	// here we know that fy.e == fz.e - 1, i.e. y >= z/2
	// TODO ??
	fy.f >>= 1;

tryComplement:
	// TODO ??
	fx.f = fz.f - fy.f;
	fx.e = fz.e;
	// use the opposite sign of y
	fx.s = '+' + '-' - fy.s;
	// $X = $Y - n * $Z, where n is the nearest integer to $Y/$Z
	// and n is an even integer in case of ties
	if(fx.f > fy.f || (fx.f == fy.f && !odd)) {
		fx.f = fy.f;
		fx.s = fy.s;
	}
	// normalize fx
	while(fx.f < 0x40000000000000) {
		fx.e--;
		fx.f <<= 1;
	}
	return fl_pack(fx,ROUND_OFF,ex);
}

static octa fl_sqrtNum(sFloat fz,int rounding,unsigned *ex) {
	sFloat fx;
	// TODO ??
	fx.f = 2;
	fx.s = '+';
	// 2^8 = 2^4*2^4
	// 2^5 = 2^2*2^2 + r
	// -> xe = _ze/2_; but we have to add the bias first!
	fx.e = (fz.e + 0x3FE) >> 1;
	// TODO ??
	if(fz.e & 1)
		fz.f <<= 1;

	// lets take a look at the example sqrt(3) in base 10:
	//      1 .  7  3  2  0
	// --------------------
	// sqrt(3 . 00 00 00 00)
	// we start with the first digit on the left. in each step we move 1 digit right
	// on the top and 2 digits right on the bottom.
	// basically we're treating them like integers and say that in each step upper*upper <= lower.
	// that means, 1*1 <= 3, 17*17 <= 300, 173*173 <= 30000 and so on.
	// of course, we're searching for upper, i.e. we search for the number so that
	// upper*upper <= lower.

	// TODO ??
	octa rf = (fz.f >> 54) - 1;
	for(int k = 53; k; k--) {
		// next digit
		rf <<= 2;
		fx.f <<= 1;
		// extract next 2 fraction-digits from z, beginning with the left-most bits
		// and add these to rf (last 2 digits are 0 before that)
		if(k >= 43)
			rf += (fz.f >> (32 + (2 * (k - 43)))) & 3;
		else if(k >= 27)
			rf += (fz.f >> (2 * (k - 27))) & 3;
		// as soon as the remainder is bigger, we set that bit in fx.f and substract fx.f from rf
		if(rf > fx.f) {
			fx.f++;
			rf -= fx.f;
			fx.f++;
		}
	}
	// sticky bit
	if(rf)
		fx.f++;
	return fl_pack(fx,rounding,ex);
}

static octa fl_addNums(sFloat fy,sFloat fz,unsigned *ex) {
	sFloat fx;
	octa o,oo;
	// we want to have: fy >= fz (ignoring sign)
	if(fy.e < fz.e || (fy.e == fz.e && fy.f < fz.f)) {
		// exchange y with z
		sFloat ft = fy;
		fy = fz;
		fz = ft;
	}
	int d = fy.e - fz.e;
	fx.e = fy.e;
	fx.s = fy.s;

	// Adjust for difference in exponents
	if(d) {
		// we have 2 bits "free" on the right. therefore if d <= 2, the result is exact
		if(d <= 2)
			fz.f >>= d;
		// more than precision?
		else if(d > 53)
			fz.f = 1;
		else {
			// TODO ??
			if(fy.s != fz.s) {
				d--;
				fx.e--;
				fy.f <<= 1;
			}
			// simply shift the fraction of z to the right so that they have the same
			// exponent
			o = fz.f;
			fz.f = o >> d;
			oo = fz.f << d;
			// if we've lost something, set the sticky bit
			if(oo != o)
				fz.f |= 1;
		}
	}

	if(fy.s == fz.s) {
		// same sign, so we can simply add the fractions
		fx.f = fy.f + fz.f;
		// if we've got one bit more on the left..
		if(fx.f >= 0x80000000000000) {
			// normalize it, i.e. increase exponent and shift fraction 1 bit right
			fx.e++;
			d = fx.f & 1;
			fx.f >>= 1;
			// set sticky bit if we've lost something
			fx.f |= d;
		}
	}
	else {
		// one of them is negative, so its a substraction
		fx.f = fy.f - fz.f;
		// same as above
		if(fx.f >= 0x80000000000000) {
			fx.e++;
			d = fx.f & 1;
			fx.f >>= 1;
			fx.f |= d;
		}
		// when substracting many bits may have been "lost", so that we have to
		// normalize the float again
		else {
			while(fx.f < 0x40000000000000) {
				fx.e--;
				fx.f <<= 1;
			}
		}
	}
	return fl_pack(fx,fl_getRoundMode(),ex);
}

static int fl_getRoundMode(void) {
	octa ra = reg_getSpecial(rA);
	int mode = (ra >> 16) & 0x3;
	return mode ? mode : ROUND_NEAR;
}

static octa fl_handleNaN(octa y,octa z,eFtype yt,eFtype zt,unsigned *ex) {
	switch(4 * yt + zt) {
		case 4 * NAN + NAN:
			// y is signaling?
			if(!(y & 0x8000000000000))
				*ex |= TRIP_FLOATINVOP;
			// fall through

		case 4 * ZERO + NAN:
		case 4 * NUM + NAN:
		case 4 * INF + NAN:
			// if z is signaling, do a TRIP and make it quiet
			if(!(z & 0x8000000000000)) {
				*ex |= TRIP_FLOATINVOP;
				z |= 0x8000000000000;
			}
			return z;

		case 4 * NAN + ZERO:
		case 4 * NAN + NUM:
		case 4 * NAN + INF:
			if(!(y & 0x8000000000000)) {
				*ex |= TRIP_FLOATINVOP;
				y |= 0x8000000000000;
			}
			return y;
	}
	// we can return 0 here as 'no NaN' because the above cases will always return the NaN
	// float. so, it will never be 0.
	return 0;
}

static octa fl_round(octa x,int sign,int rounding) {
	switch(rounding) {
		case ROUND_DOWN:
			if(sign == '-')
				x += 3;
			break;
		case ROUND_UP:
			if(sign != '-')
				x += 3;
			break;
		case ROUND_OFF:
			break;
		case ROUND_NEAR:
			x += (x & 4) ? 2 : 1;
			break;
	}
	return x;
}

// the following is an alternative implementation of fl_compareWithEps() for two ordinary numbers.
// its very close to the definition and thus easy to understand. on the other hand, its not that
// efficient as knuths solution
#if 0
// we can save work here
if(y == z)
	return ECMP_EQUAL;

// if e >= 2 all numbers are equal to each other, if we're looking for similarity
if(similarity && fe.e >= 1023)
	return ECMP_EQUAL;

// y € N(z), with respect to fe?
bool ynear = fl_isNear(y,z,fz,fe);
// if we're looking for similarity, its sufficient if one is near the other
if(similarity && ynear)
	return ECMP_EQUAL;

// z € N(y), with respect to fe?
bool znear = fl_isNear(z,y,fy,fe);
// if both are near to each other, its definitely equal
if(ynear && znear)
	return ECMP_EQUAL;
// otherwise its only equal if similarity is sufficient
if(ynear || znear)
	return similarity ? ECMP_EQUAL : ECMP_NOT_EQUAL;
return ECMP_NOT_EQUAL;

static bool fl_isNear(octa y,octa z,sFloat fz,sFloat fe) {
	unsigned ex = 0;
	// knuth defines: N(0) = {0}
	if(fz.type == ZERO)
		return (y & ~MSB(64)) == (z & ~MSB(64));
	// if e is zero, its near only if z and y are the same
	else if(fe.type == ZERO)
		return y == z;
	else {
		// fze = fe * 2^z_e
		octa zmin,zmax;
		sFloat fze = fe;
		// subnormal
		if(((z >> 52) & 0x7FF) == 0)
			fze.e -= 1021;
		// normal
		else
			fze.e += (fz.e + 1) - 1022;

		// N(z) = [z-fze .. z+fze]
		// zmin = z-fze
		// zmax = z+fze
		fze.s = '-';
		if(fz.s != fze.s && fz.e == fze.e && fz.f == fze.f)
			zmin = 0;
		else
			zmin = fl_addNums(fz,fze,&ex);
		fze.s = '+';
		if(fz.s != fze.s && fz.e == fze.e && fz.f == fze.f)
			zmax = 0;
		else
			zmax = fl_addNums(fz,fze,&ex);

		// near if: y >= zmin && y <= zmax
		octa yzmin = fl_compare(y,zmin);
		octa yzmax = fl_compare(y,zmax);
		return (yzmin == 0 || yzmin == 1) && (yzmax == 0 || yzmax == (octa)-1);
	}
}
#endif
