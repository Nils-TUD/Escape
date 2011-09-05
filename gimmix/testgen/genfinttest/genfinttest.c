/**
 * $Id: genfinttest.c 202 2011-05-10 10:49:04Z nasmussen $
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

#define RESULT_ADDR		0xF000

static uDouble numbers[] = {
	{.d = 0.},
	{.d = -0.},
	{.d = 1.},
	{.d = 2.},
	{.d = -1.5},
	{.d = -0.033123},
	{.d = 1.11},
	{.d = 1.999999},
	{.d = 0x000FFFFFFFFFFFFF},
	{.d = -0.75},
	{.d = -12.75932},
	{.d = 777.114466},
	{.o = 0x0010000000000000},	// min normal
	{.o = 0x000FFFFFFFFFFFFF},	// max subnormal
	{.o = 0x000FF00FF00FF00F},	// another subnormal
	{.o = 0x7FEFFFFFFFFFFFFF},	// max
	{.o = 0xFFEFFFFFFFFFFFFF},	// min
	{.o = 0x0000000000000001},	// pos min
	{.o = 0x8000000000000001},	// neg max
	{.o = 0x7FF0000000000000},	// +inf
	{.o = 0xFFF0000000000000},	// -inf
	{.o = 0x7FF8000000000000},	// quiet NaN
	{.o = 0x7FF4000000000000},	// signaling NaN
	{.o = 0x43E0000000000000},	// 2^63
	{.o = 0x43E1000000000000},	// > 2^63
	{.o = 0x43DFFFFFFFFFFFFF},	// 2^63 - 1024 = max. int when converting from double
	{.o = 0xC3E0000000000000},	// -2^63
	{.o = 0xC3E0000000000001},	// < -2^63
};

const sRounding *rounding;

static octa toint(uDouble d,int r);

int main(int argc,char **argv) {
	if(argc < 3)
		error("Usage: %s (mms|test) <instr> [--ex]\n",argv[0]);

	bool ex = argc > 3 && strcmp(argv[3],"--ex") == 0;

	rounding = getRoundings();
	size_t num = ARRAY_SIZE(numbers);
	if(strcmp(argv[1],"mms") == 0) {
		mprintf("%%\n");
		mprintf("%% %s.mms -- tests %s-instruction (generated)\n",argv[2],argv[2]);
		mprintf("%%\n");
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
		mprintf("		SETL	$%d,#%X\n",reg,RESULT_ADDR);
		mprintf("\n");
		reg += 2;

		mprintf("		%% Perform tests\n");
		octa loc = RESULT_ADDR;
		for(size_t r = 0; r < ROUND_COUNT; r++) {
			mprintf("		%% Set rounding mode to %s\n",rounding[r].name);
			mprintf("		SETML	$%d,#%X\n",reg - 1,rounding[r].mmix);
			mprintf("		PUT		rA,$%d\n",reg - 1);
			mprintf("\n");

			for(size_t i = 0; i < num; i++) {
				mprintf("		%s	$%d,$%d		%% (%s) %e\n",strtoupper(argv[2]),
						reg,i,strcmp(argv[2],"fint") == 0 ? "int" : "fix",numbers[i].d);
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

		mprintf("		%% Sync memory\n");
		size_t size = ROUND_COUNT * num * (ex ? 16 : 8);
		mprintf("		SETL	$%d,#%X\n",reg - 2,RESULT_ADDR);
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
					RESULT_ADDR + ROUND_COUNT * num * (ex ? 16 : 8) - 8);
		}
		else {
			int reg = 0;
			mprintf("r:$0..$%d,m:%X..%X",num - 1,RESULT_ADDR,
					RESULT_ADDR + ROUND_COUNT * num * 8 - 8);
			for(size_t i = 0; i < num; i++) {
				mprintf("\n$%d: %016OX",reg,numbers[i].o);
				reg++;
			}
			octa loc = RESULT_ADDR;
			for(size_t r = 0; r < ROUND_COUNT; r++) {
				for(size_t i = 0; i < num; i++) {
					uDouble res;
					if(strcmp(argv[2],"fint") == 0)
						res.d = rounding[r].cFunc(numbers[i].d);
					else
						res.o = toint(numbers[i],r);

					mprintf("\nm[%016OX]: %016OX",loc,res.o);
					loc += 8;
				}
			}
		}
	}

	return EXIT_SUCCESS;
}

static octa toint(uDouble d,int r) {
	uDouble res;
	// NaN and INF stay the same
	if(isNaN(d.o) || isInf(d.o))
		res.o = d.o;
	// these cases are handled differently in x86, therefore do it like we do it in mmix
	else if(d.d > ~((octa)1 << 63) || d.d < (socta)((octa)1 << 63)) {
		octa f = ((d.o & 0xFFFFFFFFFFFFF) << 2) | 0x40000000000000;
		int e = ((d.o >> 52) & 0x7FF) - 1;
		if(e >= 1140)
			res.o = 0;
		else {
			res.o = f << (e - 1076);
			if(d.o & ((octa)1 << 63))
				res.o = 0 - res.o;
		}
	}
	// the normal cases are the same
	else {
		res.d = rounding[r].cFunc(d.d);
		if((res.o & 0x7FFFFFFFFFFFFFFF) == 0)
			res.o = 0;
		else
			res.o = (socta)res.d;
	}
	return res.o;
}
