/**
 * $Id: specregs.c 239 2011-08-30 18:28:06Z nasmussen $
 */

#include <string.h>
#include <time.h>

#include "common.h"
#include "core/register.h"
#include "core/mmu.h"
#include "cli/lang/specreg.h"
#include "mmix/io.h"

#define MAX_EXPLAIN_LEN		256

static const char *sreg_explainrV(octa bits);
static const char *sreg_getSegmentSize(const int *b,int segment);
static const char *sreg_explainrU(octa bits);
static const char *sreg_explainrN(octa bits);
static const char *sreg_explainrA(octa bits);
static const char *sreg_explainrQrK(octa bits);
static char *sreg_explainBits(char *buf,const char **table,int tableSize,octa bits);

static const sSpecialReg specialRegs[] = {
	/* 00 */	{"rB",	"bootstrap (trip)"},
	/* 01 */	{"rD",	"dividend"},
	/* 02 */	{"rE",	"epsilon"},
	/* 03 */	{"rH",	"himult"},
	/* 04 */	{"rJ",	"return-jump"},
	/* 05 */	{"rM",	"multiplex mask"},
	/* 06 */	{"rR",	"remainder"},
	/* 07 */	{"rBB",	"bootstrap (trap)"},
	/* 08 */	{"rC",	"cycle counter"},
	/* 09 */	{"rN",	"serial number"},
	/* 0A */	{"rO",	"register stack offset"	},
	/* 0B */	{"rS",	"register stack pointer"},
	/* 0C */	{"rI",	"interval counter"},
	/* 0D */	{"rT",	"trap address"},
	/* 0E */	{"rTT",	"dynamic trap address"},
	/* 0F */	{"rK",	"interrupt mask"},
	/* 10 */	{"rQ",	"interrupt request"},
	/* 11 */	{"rU",	"usage counter"},
	/* 12 */	{"rV",	"virtual translation"},
	/* 13 */	{"rG",	"global threshold"},
	/* 14 */	{"rL",	"local threshold"},
	/* 15 */	{"rA",	"arithmetic status"},
	/* 16 */	{"rF",	"failure location"},
	/* 17 */	{"rP",	"prediction"},
	/* 18 */	{"rW",	"where-interrupted (trip)"},
	/* 19 */	{"rX",	"execution (trip)"},
	/* 1A */	{"rY",	"Y operand (trip)"},
	/* 1B */	{"rZ",	"Z operand (trip)"},
	/* 1C */	{"rWW",	"where-interrupted (trap)"},
	/* 1D */	{"rXX",	"execution (trap)"},
	/* 1E */	{"rYY",	"Y operand (trap)"},
	/* 1F */	{"rZZ",	"Z operand (trap)"},
	/* 20 */	{"rSS",	"kernel-stack"}
};
static const char *trapBits[] = {
	/* 00..07 */ "P","M","N","R","?","?","?","I",
	/* 08..15 */ "?","?","?","?","?","?","?","?",
	/* 16..23 */ "?","?","?","?","?","?","?","?",
	/* 24..31 */ "?","?","?","?","?","?","?","?",
	/* 32..39 */ "p","s","b","k","n","x","w","r",
	/* 40..47 */ "?","?","?","?","?","?","?","?",
	/* 48..55 */ "?","?","?","D","T","T0R","T0T","T1R",
	/* 56..63 */ "T1T","T2R","T2T","T3R","T3T","?","?","?",
};
static const char *raBits[] = {
	/* 00..07 */ "X","Z","U","O","I","W","V","D"
};
static const char *roundMode[] = {
	/* 00..03 */ "near","off","up","down"
};
static char buffer[MAX_EXPLAIN_LEN];

const char *sreg_explain(int rno,octa bits) {
	switch(rno) {
		case rA:
			return sreg_explainrA(bits);
		case rK:
		case rQ:
			return sreg_explainrQrK(bits);
		case rN:
			return sreg_explainrN(bits);
		case rU:
			return sreg_explainrU(bits);
		case rV:
			return sreg_explainrV(bits);
	}
	return NULL;
}

const sSpecialReg *sreg_get(int rno) {
	if(rno >= 0 && rno < (int)ARRAY_SIZE(specialRegs))
		return specialRegs + rno;
	return NULL;
}

int sreg_getByName(const char *name) {
	for(int i = 0; i < (int)ARRAY_SIZE(specialRegs); i++) {
		if(strcmp(specialRegs[i].name,name) == 0)
			return i;
	}
	return -1;
}

static const char *sreg_explainrV(octa bits) {
	const sVirtTransReg vtr = mmu_unpackrV(bits);
	msnprintf(buffer,sizeof(buffer),
			"Pages: s[0]=%s, s[1]=%s, s[2]=%s, s[3]=%s, Pagesize: 2^%d, "
			"Root: #%08OX, n: #%X, transl.: %s%s",
			sreg_getSegmentSize(vtr.b,0),sreg_getSegmentSize(vtr.b,1),
			sreg_getSegmentSize(vtr.b,2),sreg_getSegmentSize(vtr.b,3),
			vtr.s,vtr.r,vtr.n,vtr.f ? "sw" : "hw",
			(vtr.s < 13 || vtr.s > 48 || vtr.f > 1) ? " (invalid)" : "");
	return buffer;
}

static const char *sreg_getSegmentSize(const int *b,int segment) {
	static char size[15];
	if(b[segment] > b[segment + 1])
		return "0";
	if(b[segment] == b[segment + 1])
		return "1";
	msnprintf(size,sizeof(size),"2^%d",10 * (b[segment + 1] - b[segment]));
	return size;
}

static const char *sreg_explainrU(octa bits) {
	byte upattern = bits >> 56;
	byte umask = (bits >> 48) & 0xFF;
	int priv = bits & 0x800000000000;
	octa counter = bits & 0x7FFFFFFFFFFF;
	msnprintf(buffer,sizeof(buffer),"Pattern: #%02BX, Mask: #%02BX, Priv: %d, Counter: #%012OX",
			upattern,umask,priv ? 1 : 0,counter);
	return buffer;
}

static const char *sreg_explainrN(octa bits) {
	char *buf = buffer;
	time_t ts = bits & 0xFFFFFFFF;
	struct tm *pTime = gmtime(&ts);
	strftime(buf,sizeof(buffer),"Build on %c %Z, Version ",pTime);
	while(*buf)
		buf++;
	int version = bits >> 56;
	int subversion = (bits >> 48) & 0xFF;
	int subsubversion = (bits >> 40) & 0xFF;
	msnprintf(buf,sizeof(buffer) - (buf - buffer),"%d.%d.%d",version,subversion,subsubversion);
	return buffer;
}

static const char *sreg_explainrA(octa bits) {
	char *buf = buffer;
	const char *mode = roundMode[(bits >> 16) & 0x3];
	while(*mode)
		*buf++ = *mode++;
	*buf++ = ':';
	// add enable-bits
	buf = sreg_explainBits(buf,raBits,ARRAY_SIZE(raBits),(bits >> 8) & 0xFF);
	*buf++ = ':';
	// add event-bits
	sreg_explainBits(buf,raBits,ARRAY_SIZE(raBits),bits & 0xFF);
	return buffer;
}

static const char *sreg_explainrQrK(octa bits) {
	sreg_explainBits(buffer,trapBits,ARRAY_SIZE(trapBits),bits);
	return buffer;
}

static char *sreg_explainBits(char *buf,const char **table,int tableSize,octa bits) {
	int i = 0;
	for(int j = tableSize - 1; j >= 0; j--) {
		if((bits & ((octa)1 << j)) && table[j][0] != '?') {
			const char *abbrev = table[j];
			while(*abbrev)
				*buf++ = *abbrev++;
			*buf++ = '+';
			i++;
		}
	}
	if(i > 0)
		buf--;
	else
		*buf++ = '-';
	*buf = '\0';
	return buf;
}
