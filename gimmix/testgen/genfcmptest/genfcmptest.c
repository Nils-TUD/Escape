/**
 * $Id: genfcmptest.c 200 2011-05-08 15:40:08Z nasmussen $
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <float.h>

#include "common.h"
#include "mmix/io.h"
#include "mmix/error.h"
#include "test/util.h"

#define RESULT_ADDR		0xF000

static uDouble numbers[] = {
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
};

int main(int argc,char **argv) {
	if(argc < 3)
		error("Usage: %s (mms|test) (fcmp|feql|fun) [--ex]\n",argv[0]);

	bool ex = argc > 3 && strcmp(argv[3],"--ex") == 0;

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

		mprintf("		%% Perform %s tests\n",argv[2]);
		octa loc = RESULT_ADDR;
		for(size_t i = 0; i < num; i++) {
			for(size_t j = 0; j < num; j++) {
				mprintf("		%s	$%d,$%d,$%d		%% %e <> %e\n",strtoupper(argv[2]),reg,i,j,
						numbers[i].d,numbers[j].d);
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
		size_t size = num * num * (ex ? 16 : 8);
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
			mprintf("m:%X..%X",RESULT_ADDR,RESULT_ADDR + num * num * (ex ? 16 : 8) - 8);
		}
		else {
			int reg = 0;
			mprintf("r:$0..$%d,m:%X..%X",num - 1,RESULT_ADDR,RESULT_ADDR + num * num * 8 - 8);
			for(size_t i = 0; i < num; i++) {
				mprintf("\n$%d: %016OX",reg,numbers[i].o);
				reg++;
			}
			octa loc = RESULT_ADDR;
			for(size_t i = 0; i < num; i++) {
				for(size_t j = 0; j < num; j++) {
					octa res;
					if(isNaN(numbers[i].o) || isNaN(numbers[j].o))
						res = strcmp(argv[2],"fun") == 0 ? 1 : 0;
					else {
						res = numbers[i].d < numbers[j].d ? -1 :
							numbers[i].d == numbers[j].d ? 0 : 1;
						if(strcmp(argv[2],"feql") == 0)
							res = res == 0 ? 1 : 0;
						else if(strcmp(argv[2],"fun") == 0)
							res = 0;
					}

					mprintf("\nm[%016OX]: %016OX",loc,res);
					loc += 8;
				}
			}
		}
	}

	return EXIT_SUCCESS;
}
