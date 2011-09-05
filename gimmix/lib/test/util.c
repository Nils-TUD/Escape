/**
 * $Id: util.c 152 2011-04-05 18:49:38Z nasmussen $
 */

#include <ctype.h>
#include <float.h>
#include <math.h>
#include <fpu_control.h>

#include "common.h"
#include "test/util.h"

static sRounding rounding[] = {
	{0,	round,	_FPU_RC_NEAREST,"NEAR"},
	{3,	floor,	_FPU_RC_DOWN,	"DOWN"},
	{2,	ceil,	_FPU_RC_UP,		"UP"},
	{1,	trunc,	_FPU_RC_ZERO,	"ZERO"}
};

const sRounding *getRoundings(void) {
	return rounding;
}

void setFPUCW(unsigned int mode) {
    fpu_control_t cw;
    _FPU_GETCW(cw);
    cw &= ~_FPU_EXTENDED;
    cw |= _FPU_DOUBLE;
    cw &= ~(_FPU_RC_DOWN | _FPU_RC_NEAREST | _FPU_RC_UP | _FPU_RC_ZERO);
    cw |= mode;
    _FPU_SETCW(cw);
}

bool isInf(octa x) {
	return (x & 0x7FFFFFFFFFFFFFFF) == 0x7FF0000000000000;
}

bool isZero(octa x) {
	return (x & 0x7FFFFFFFFFFFFFFF) == 0;
}

bool isNaN(octa x) {
	return ((x >> 52) & 0x7FF) == 0x7FF && (x & 0xFFFFFFFFFFFFF) != 0;
}

bool isSignalingNaN(octa x) {
	return isNaN(x) && (x & 0x8000000000000) == 0;
}

octa setSign(octa x,bool set) {
	x &= ~((octa)1 << 63);
	if(set)
		x |= (octa)1 << 63;
	return x;
}

uDouble correctResForNaNOps(uDouble res,uDouble y,uDouble z) {
	// wikipedia says:
	//   Das Vorzeichenbit hat bei NaN keine Bedeutung. Es ist nicht
	//   spezifiziert, welchen Wert das Vorzeichenbit bei zurückgegebenen NaN
	//   besitzt.
	// unfortunatly, mmix and x86 behave differently, therefore we have to
	// adjust it
	if(isNaN(y.o) && isNaN(z.o)) {
		res.o = z.o;
		if(isSignalingNaN(z.o))
			res.o |= 0x8000000000000;
	}
	else if(isNaN(y.o)) {
		res.o = y.o;
		if(isSignalingNaN(y.o))
			res.o |= 0x8000000000000;
	}
	else if(isNaN(z.o)) {
		res.o = z.o;
		if(isSignalingNaN(z.o))
			res.o |= 0x8000000000000;
	}
	return res;
}

char *strtoupper(char *s) {
	char *begin = s;
	while(*s) {
		*s = toupper(*s);
		s++;
	}
	return begin;
}
