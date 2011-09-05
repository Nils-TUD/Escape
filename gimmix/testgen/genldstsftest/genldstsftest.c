/**
 * $Id: genldstsftest.c 200 2011-05-08 15:40:08Z nasmussen $
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

#define TMP_ADDR		0xE000
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
	{.d = 1.4E-45},				// smallest unnormalized 32-bit float
	{.d = 1.2E-45},
	{.d = 3E-40},
	{.o = 0x3FFFFFFFFFFFFFFF},	// 1.9999...
	{.o = 0x7FEFFFFFFFFFFFFF},	// max
	{.o = 0xFFEFFFFFFFFFFFFF},	// min
	{.o = 0x0000000000000001},	// pos min
	{.o = 0x8000000000000001},	// neg max
	{.o = 0x7FF0000000000000},	// +inf
	{.o = 0xFFF0000000000000},	// -inf
	{.o = 0x7FF8000000000000},	// quiet NaN
	{.o = 0x7FF4000000000000},	// signaling NaN
};

int main(int argc,char **argv) {
	if(argc < 2)
		error("Usage: %s (mms|test) <instr> [--ex]\n",argv[0]);

	bool ex = argc > 2 && strcmp(argv[2],"--ex") == 0;

	const sRounding *rounding = getRoundings();
	size_t num = ARRAY_SIZE(numbers);
	if(strcmp(argv[1],"mms") == 0) {
		mprintf("%%\n");
		mprintf("%% ldstsf.mms -- tests ldsf and stsf (generated)\n");
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
		mprintf("		SETL	$%d,#%X\n",reg,TMP_ADDR);
		mprintf("		SETL	$%d,#%X\n",reg + 1,RESULT_ADDR);
		mprintf("\n");
		reg += 3;

		mprintf("		%% Perform tests\n");
		octa loc = RESULT_ADDR;
		for(size_t r = 0; r < ROUND_COUNT; r++) {
			mprintf("		%% Set rounding mode to %s\n",rounding[r].name);
			mprintf("		SETML	$%d,#%X\n",reg - 1,rounding[r].mmix);
			mprintf("		PUT		rA,$%d\n",reg - 1);
			mprintf("\n");

			for(size_t i = 0; i < num; i++) {
				mprintf("		STSF	$%d,$%d,0		%% %e -> #%OX\n",
						i,reg - 3,numbers[i].d,(octa)TMP_ADDR);
				if(ex) {
					mprintf("		GET		$%d,rA\n",reg);
					mprintf("		STOU	$%d,$%d,0		%% #%OX\n",reg,reg - 2,loc);
					mprintf("		ADDU	$%d,$%d,%d\n",reg - 2,reg - 2,8);
					mprintf("		PUT		rA,$%d\n",reg - 1);
					loc += 8;
				}

				mprintf("		LDSF	$%d,$%d,0		%% $%d <- %e\n",
						reg,reg - 3,reg,numbers[i].d);
				mprintf("		STOU	$%d,$%d,0		%% #%OX\n",reg,reg - 2,loc);
				mprintf("		ADDU	$%d,$%d,%d\n",reg - 2,reg - 2,8);
				loc += 8;
				if(ex) {
					mprintf("		GET		$%d,rA\n",reg);
					mprintf("		STOU	$%d,$%d,0		%% #%OX\n",reg,reg - 2,loc);
					mprintf("		ADDU	$%d,$%d,%d\n",reg - 2,reg - 2,8);
					mprintf("		PUT		rA,$%d\n",reg - 1);
					loc += 8;
				}
				mprintf("\n");
			}
		}

		mprintf("		%% Sync memory\n");
		size_t size = ROUND_COUNT * num * (ex ? 24 : 8);
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
				setFPUCW(rounding[r].x86);

				for(size_t i = 0; i < num; i++) {
					uDouble res;
					// again, strange behaviour of x86: if its > FLT_MAX the result is FLT_MAX
					// not INF. the same for < -FLT_MAX.
					if(numbers[i].d > FLT_MAX)
						res.o = 0x7FF0000000000000;
					else if(numbers[i].d < -FLT_MAX)
						res.o = 0xFFF0000000000000;
					else
						res.d = (float)(numbers[i].d);

					mprintf("\nm[%016OX]: %016OX",loc,res.o);
					loc += 8;
				}
			}
		}
	}

	return EXIT_SUCCESS;
}
