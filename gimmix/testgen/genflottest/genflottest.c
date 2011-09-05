/**
 * $Id: genflottest.c 200 2011-05-08 15:40:08Z nasmussen $
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

static octa numbers[] = {
	0,1,2,42,8588381821223311223,400000000,11111111,
	0x7FFFFFFFFFFFFFFF,0x8000000000000000,
	-1,-2,-231,-18288828222222222
};

int main(int argc,char **argv) {
	if(argc < 3)
		error("Usage: %s (mms|test) <instr> [--ex]\n",argv[0]);

	bool ex = argc > 3 && strcmp(argv[3],"--ex") == 0;

	const sRounding *rounding = getRoundings();
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
			mprintf("		SETH	$%d,#%04OX\n",reg,(numbers[i] >> 48) & 0xFFFF);
			mprintf("		ORMH	$%d,#%04OX\n",reg,(numbers[i] >> 32) & 0xFFFF);
			mprintf("		ORML	$%d,#%04OX\n",reg,(numbers[i] >> 16) & 0xFFFF);
			mprintf("		ORL		$%d,#%04OX\n",reg,(numbers[i] >>  0) & 0xFFFF);
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
				mprintf("		%s	$%d,$%d		%% (%s) #%OX\n",strtoupper(argv[2]),
						reg,i,argv[2],numbers[i]);
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
				mprintf("\n$%d: %016OX",reg,numbers[i]);
				reg++;
			}
			octa loc = RESULT_ADDR;
			for(size_t r = 0; r < ROUND_COUNT; r++) {
				setFPUCW(rounding[r].x86);

				for(size_t i = 0; i < num; i++) {
					uDouble res;
					if(strcmp(argv[2],"flot") == 0 || strcmp(argv[2],"sflot") == 0)
						res.d = (socta)numbers[i];
					else
						res.d = numbers[i];
					if(argv[2][0] == 's')
						res.d = (float)res.d;

					mprintf("\nm[%016OX]: %016OX",loc,res.o);
					loc += 8;
				}
			}
		}
	}

	return EXIT_SUCCESS;
}
