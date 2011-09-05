/**
 * $Id: gendiv.c 200 2011-05-08 15:40:08Z nasmussen $
 */

#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "common.h"
#include "mmix/io.h"
#include "mmix/error.h"

static socta divF(socta D,socta d);
static socta modF(socta D,socta d);

static octa numbers[] = {
	0,-1,-3,1,3,5,10,42,1024,0x8000000000000000,0x7FFFFFFFFFFFFFFF
};

int main(int argc,char **argv) {
	bool useDiv = false;
	if(argc < 2)
		error("Usage: %s (mms|test) --signed\n",argv[0]);

	if(argc > 2)
		useDiv = strcmp(argv[2],"--signed") == 0;

	size_t num = ARRAY_SIZE(numbers);
	if(strcmp(argv[1],"mms") == 0) {
		mprintf("%%\n");
		mprintf("%% %s.mms -- tests %s-instruction (generated)\n",useDiv ? "div" : "divu",
				useDiv ? "div" : "divu");
		mprintf("%%\n");
		mprintf("		LOC		#1000\n");
		mprintf("\n");
		mprintf("Main	OR		$0,$0,$0	%% dummy\n");
		mprintf("\n");

		mprintf("		%% Put numbers in registers\n");
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
		mprintf("		SETL	$%d,#8000\n",reg);
		mprintf("\n");
		reg++;

		mprintf("		%% Perform div tests\n");
		octa loc = 0x8000;
		for(size_t i = 0; i < num; i++) {
			for(size_t j = 0; j < num; j++) {
				mprintf("		%s	$%d,$%d,$%d		%% #%016OX / #%016OX\n",useDiv ? "DIV" : "DIVU",
						reg,i,j,numbers[i],numbers[j]);
				mprintf("		STOU	$%d,$%d,0		%% #%OX\n",reg,reg - 1,loc);
				mprintf("		GET		$%d,rR\n",reg);
				mprintf("		STOU	$%d,$%d,8		%% #%OX\n",reg,reg - 1,loc + 8);
				mprintf("		GET		$%d,rA\n",reg);
				mprintf("		STOU	$%d,$%d,16		%% #%OX\n",reg,reg - 1,loc + 16);
				mprintf("		PUT		rA,0\n");
				mprintf("		ADDU	$%d,$%d,24\n",reg - 1,reg - 1);
				mprintf("\n");
				loc += 24;
			}
		}

		mprintf("		%% Sync memory\n");
		size_t size = num * num * 24;
		mprintf("		SETL	$%d,#8000\n",reg - 1);
		while(size > 0) {
			size_t amount = MIN(0xFE,size);
			mprintf("		SYNCD	#%X,$%d\n",amount,reg - 1);
			mprintf("		ADDU	$%d,$%d,#%X\n",reg - 1,reg - 1,amount + 1);
			size -= amount;
		}
	}
	else {
		int reg = 0;
		mprintf("r:$0..$%d,m:8000..%X",num - 1,0x8000 + num * num * 24 - 8);
		for(size_t i = 0; i < num; i++) {
			mprintf("\n$%d: %016OX",reg,numbers[i]);
			reg++;
		}
		octa loc = 0x8000;
		for(size_t i = 0; i < num; i++) {
			for(size_t j = 0; j < num; j++) {
				octa divres,rem,ra;
				ra = 0;
				if(numbers[j] == 0) {
					if(useDiv)
						ra |= 0x80;
					divres = 0;
					rem = numbers[i];
				}
				else if(useDiv && numbers[i] == ((octa)1 << 63) && numbers[j] == (octa)-1) {
					ra |= 0x40;
					divres = numbers[i];
					rem = 0;
				}
				else {
					if(useDiv) {
						divres = divF(numbers[i],numbers[j]);
						rem = modF(numbers[i],numbers[j]);
					}
					else {
						divres = numbers[i] / numbers[j];
						rem = numbers[i] % numbers[j];
					}
				}

				mprintf("\nm[%016OX]: %016OX",loc,divres);
				mprintf("\nm[%016OX]: %016OX",loc + 8,rem);
				mprintf("\nm[%016OX]: %016OX",loc + 16,ra);
				loc += 24;
			}
		}
	}

	return EXIT_SUCCESS;
}

// source: doc/divmodnote.pdf
// from x86-signed-division to mmix-signed-division ("floored division")
static socta divF(socta D,socta d) {
	socta q = D / d;
	socta r = D % d;
	if((r > 0 && d < 0) || (r < 0 && d > 0))
		q = q - 1;
	return q;
}

// source: doc/divmodnote.pdf
// from x86-signed-modulo to mmix-signed-modulo
static socta modF(socta D,socta d) {
	socta r = D % d;
	if((r > 0 && d < 0) || (r < 0 && d > 0))
		r = r + d;
	return r;
}
