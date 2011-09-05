/**
 * $Id: genfcmpetest.c 200 2011-05-08 15:40:08Z nasmussen $
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <float.h>
#include <math.h>

#include "common.h"
#include "mmix/io.h"
#include "mmix/error.h"
#include "test/util.h"

#define RESULT_ADDR		0x40000

static uDouble eps[] = {
	{.d = DBL_EPSILON},
	{.d = 0.1},
	{.d = 1.},
	{.d = 2.},
	{.d = 0},
	{.d = -1},
	{.o = 0x7FF8000000000000},	// quiet NaN
	{.o = 0x7FF0000000000000},	// +inf
};

static uDouble numbers[] = {
	{.d = DBL_EPSILON},
	{.d = 0.},
	{.d = 1.},
	{.d = 2.},
	{.d = -1.},
	{.d = -0.},
	{.d = 0.5},
	{.d = 0.75},
	{.d = 0.4},
	{.d = -0.75},
	{.d = -12.75932},
	{.d = 777.114466},
	{.o = 0x3FE0000000100000},
	{.o = 0x3FE0000001000000},
	{.o = 0x3FE0000000010000},
	{.o = 0x3FE0000000020000},
	{.o = 0x0000000000000002},
	{.o = 0x0000000000000003},
	{.o = 0x7FEFFFFFFFFFFFFF},	// max
	{.o = 0xFFEFFFFFFFFFFFFF},	// min
	{.o = 0x0000000000000001},	// pos min
	{.o = 0x8000000000000001},	// neg max
	{.o = 0x7FF0000000000000},	// +inf
	{.o = 0xFFF0000000000000},	// -inf
	{.o = 0x7FF8000000000000},	// quiet NaN
	{.o = 0x7FF4000000000000},	// signaling NaN
};

static octa compare(uDouble y,uDouble z,uDouble e,bool similarity);
static bool isNear(uDouble y,uDouble z,uDouble e);
static int getExp(uDouble d);

int main(int argc,char **argv) {
	if(argc < 3)
		error("Usage: %s (mms|test) (fcmpe|feqle|fune) [--ex]\n",argv[0]);

	bool ex = argc > 3 && strcmp(argv[3],"--ex") == 0;

	size_t num = ARRAY_SIZE(numbers);
	if(strcmp(argv[1],"mms") == 0) {
		mprintf("%%\n");
		mprintf("%% %s.mms -- tests %s-instruction (generated)\n",argv[2],argv[2]);
		mprintf("%%\n");
		mprintf("\n");
		mprintf("		LOC		#1000\n");
		mprintf("\n");
		mprintf("Main	OR		$0,$0,$0	%% dummy\n");
		mprintf("		PUT		rA,0\n");
		mprintf("\n");

		mprintf("		%% Put floats in registers\n");
		int reg = 0;
		for(size_t i = 0; i < num; i++) {
			mprintf("		SETH	$%d,#%04OX\n",reg,(numbers[i].o >> 48) & 0xFFFF);
			mprintf("		ORMH	$%d,#%04OX\n",reg,(numbers[i].o >> 32) & 0xFFFF);
			mprintf("		ORML	$%d,#%04OX\n",reg,(numbers[i].o >> 16) & 0xFFFF);
			mprintf("		ORL		$%d,#%04OX\n",reg,(numbers[i].o >>  0) & 0xFFFF);
			mprintf("\n");
			reg++;
		}

		mprintf("		%% Setup location for results\n");
		mprintf("		SETML	$%d,#%X\n",reg,(RESULT_ADDR >> 16) & 0xFFFF);
		mprintf("		ORL		$%d,#%X\n",reg,(RESULT_ADDR >>  0) & 0xFFFF);
		mprintf("\n");
		reg += 2;

		mprintf("		%% Perform %s tests\n",argv[2]);
		octa loc = RESULT_ADDR;
		for(size_t e = 0; e < ARRAY_SIZE(eps); e++) {
			// always compare with current rE
			numbers[0].d = eps[e].d;
			mprintf("		%% Set rE to %e\n",eps[e].d);
			mprintf("		SETH	$0,#%04OX\n",(eps[e].o >> 48) & 0xFFFF);
			mprintf("		ORMH	$0,#%04OX\n",(eps[e].o >> 32) & 0xFFFF);
			mprintf("		ORML	$0,#%04OX\n",(eps[e].o >> 16) & 0xFFFF);
			mprintf("		ORL		$0,#%04OX\n",(eps[e].o >>  0) & 0xFFFF);
			mprintf("		PUT		rE,$0\n");
			mprintf("\n");

			for(size_t i = 0; i < num; i++) {
				for(size_t j = 0; j < num; j++) {
					mprintf("		%s	$%d,$%d,$%d		%% %e <> %e (%e)\n",
							strtoupper(argv[2]),reg,i,j,numbers[i].d,numbers[j].d,eps[e].d);
					mprintf("		STOU	$%d,$%d,0		%% #%OX\n",reg,reg - 2,loc);
					if(ex) {
						mprintf("		GET		$%d,rA\n",reg);
						mprintf("		STOU	$%d,$%d,8		%% #%OX\n",reg,reg - 2,loc + 8);
						mprintf("		PUT		rA,$%d\n",reg - 1);
					}
					mprintf("		ADDU	$%d,$%d,%d\n",reg - 2,reg - 2,ex ? 16 : 8);
					mprintf("\n");
					loc += ex ? 16 : 8;
				}
			}
		}

		mprintf("		%% Sync memory\n");
		size_t size = ARRAY_SIZE(eps) * num * num * (ex ? 16 : 8);
		mprintf("		SETML	$%d,#%X\n",reg - 2,(RESULT_ADDR >> 16) & 0xFFFF);
		mprintf("		ORL		$%d,#%X\n",reg - 2,(RESULT_ADDR >>  0) & 0xFFFF);
		while(size > 0) {
			size_t amount = MIN(0xFE,size);
			mprintf("		SYNCD	#%X,$%d\n",amount,reg - 2);
			mprintf("		ADDU	$%d,$%d,#%X\n",reg - 2,reg - 2,amount + 1);
			size -= amount;
		}
	}
	else {
		// no expected results if exceptions should be compared
		if(ex) {
			mprintf("m:%X..%X",RESULT_ADDR,
					RESULT_ADDR + ARRAY_SIZE(eps) * num * num * (ex ? 16 : 8) - 8);
		}
		else {
			int reg = 1;
			mprintf("r:$1..$%d,m:%X..%X",num - 1,RESULT_ADDR,
					RESULT_ADDR + ARRAY_SIZE(eps) * num * num * 8 - 8);
			for(size_t i = 1; i < num; i++) {
				mprintf("\n$%d: %016OX",reg,numbers[i].o);
				reg++;
			}
			octa loc = RESULT_ADDR;
			for(size_t e = 0; e < ARRAY_SIZE(eps); e++) {
				numbers[0].d = eps[e].d;
				for(size_t i = 0; i < num; i++) {
					for(size_t j = 0; j < num; j++) {
						octa res = compare(numbers[i],numbers[j],eps[e],
								strcmp(argv[2],"feqle") != 0);
						if(strcmp(argv[2],"feqle") == 0)
							res = res == 0 ? 1 : 0;
						else if(strcmp(argv[2],"fune") == 0)
							res = res == 2 ? 1 : 0;
						else
							res = res == 2 ? 0 : res;
						mprintf("\nm[%016OX]: %016OX",loc,res);
						loc += 8;
					}
				}
			}
		}
	}

	return EXIT_SUCCESS;
}

static octa compare(uDouble y,uDouble z,uDouble e,bool similarity) {
	octa res = 3;
	if(e.d < 0 || isNaN(e.o) || isNaN(y.o) || isNaN(z.o))
		res = 2;
	else if(isInf(y.o) && isInf(z.o)) {
		if(((y.o ^ z.o) >> 63) == 0 || e.d >= 2)
			res = 0;
		else
			res = 3;
	}
	else if(isInf(y.o) || isInf(z.o))
		res = (similarity && e.d >= 1) ? 0 : 3;
	else if(isZero(y.o) && isZero(z.o))
		res = 0;
	// knuth defines, that zero and not-zero is never equal with respect to e
	else if(!similarity && isZero(y.o) != isZero(z.o))
		res = 3;

	if(res == 3) {
		if(similarity && e.d >= 2)
			res = 0;
		else if(isInf(y.o) || isInf(z.o))
			res = y.o == z.o ? 0 : (y.d < z.d ? -1 : 1);
		else {
			bool ynearz = isNear(y,z,e);
			bool zneary = isNear(z,y,e);
			if(similarity && (ynearz || zneary))
				res = 0;
			else if(ynearz && zneary)
				res = 0;
			else
				res = y.d < z.d ? -1 : 1;
		}
	}
	return res;
}

static bool isNear(uDouble y,uDouble z,uDouble e) {
	if(isZero(z.o))
		return isZero(y.o) && isZero(z.o);
	else {
		int zexp = getExp(z);
		long double epsi = powl(2,zexp - 1022) * e.d;
		uDouble min = {.o = 1};
		return (epsi >= min.d || y.o == z.o) &&
				y.d >= (z.d - epsi) && y.d <= (z.d + epsi);
	}
}

static int getExp(uDouble d) {
	int dexp = (d.o >> 52) & 0x7FF;
	// 1 - 1022 = -1021
	if(dexp == 0)
		return 1;
	return dexp;
}
