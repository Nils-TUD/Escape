/**
 * $Id: print.c 153 2011-04-06 16:46:17Z nasmussen $
 */

#include <string.h>
#include <ctype.h>

#include "common.h"
#include "cli/cmds.h"
#include "cli/cmd/print.h"
#include "core/register.h"
#include "core/mmu.h"
#include "core/arith/float.h"
#include "core/cpu.h"
#include "cli/lang/specreg.h"
#include "mmix/io.h"

#define MAX_FMT_LEN	16

#define PFMT_UDEC	0
#define PFMT_SDEC	1
#define PFMT_OCT	2
#define PFMT_HEX	3
#define PFMT_FLOAT	4
#define PFMT_STR	5

#define PFMT_BYTE	0
#define PFMT_WYDE	1
#define PFMT_TETRA	2
#define PFMT_OCTA	3

typedef struct {
	int format;
	const char *name;
	const char *pffmt;
} sPrintFormat;

static void printChar(int c);
static const sPrintFormat *getPrintFormat(int type,const char *fmt);

static const char *usage = "Usage: p [<fmt>] <expr>...\n"
		"  <fmt> may be:\n"
		"    u  - decimal unsigned\n"
		"    d  - decimal signed\n"
		"    o  - octal unsigned\n"
		"    x  - hexadecimal unsigned\n"
		"    lx - hexadecimal unsigned, 16 digits\n"
		"    f  - floating point number\n"
		"    s  - string\n";

static const sPrintFormat printFormats[] = {
	{PFMT_HEX,		"",		"%016OX"},	// the default one
	{PFMT_UDEC,		"u",	"%Ou"},
	{PFMT_SDEC,		"d",	"%Od"},
	{PFMT_OCT,		"o",	"%Oo"},
	{PFMT_OCT,		"o",	"%Oo"},
	{PFMT_HEX,		"x",	"%OX"},
	{PFMT_HEX,		"lx",	"%016OX"},
	{PFMT_FLOAT,	"f",	"%g"},
	{PFMT_STR,		"s",	"%s"},
};

void cli_cmd_print(size_t argc,const sASTNode **argv) {
	static char sformat[MAX_FMT_LEN];
	if(argc == 0)
		cmds_throwEx(usage);

	// read first arg (maybe the format)
	size_t i = 0;
	sCmdArg first = cmds_evalArg(argv[0],ARGVAL_ALL,0,NULL);
	const char *fmtname = first.type == ARGVAL_STR ? first.d.str : NULL;
	if(first.type == ARGVAL_STR)
		i++;

	for(; i < argc; i++) {
		for(octa offset = 0; ; offset++) {
			bool finished = false;
			sCmdArg expr = cmds_evalArg(argv[i],ARGVAL_ALL,offset,&finished);
			if(finished)
				break;

			int origin = expr.origin;
			octa loc = expr.location;
			octa value = expr.d.integer;

			switch(origin) {
				case ORG_VMEM:
				case ORG_VMEM1:
				case ORG_VMEM2:
				case ORG_VMEM4:
				case ORG_VMEM8:
					mprintf("M[%016OX]: ",loc);
					break;
				case ORG_PMEM:
					mprintf("m[%016OX]: ",loc);
					break;
				case ORG_LREG:
					mprintf("l[%d]: ",(int)loc);
					break;
				case ORG_GREG:
					mprintf("g[%d]: ",(int)loc);
					break;
				case ORG_SREG:
					mprintf("sp[%3s]: ",sreg_get(loc)->name,(int)loc);
					break;
				case ORG_DREG:
					mprintf("$%d: ",(int)loc);
					break;
			}

			const sPrintFormat *fmt = getPrintFormat(expr.type,fmtname);
			if(expr.type == ARGVAL_STR) {
				const char *s = expr.d.str;
				while(*s) {
					if(fmt->format == PFMT_STR)
						printChar(*s);
					else if(fmt->format == PFMT_FLOAT)
						mprintf("%g ",(double)*s);
					else {
						msnprintf(sformat,sizeof(sformat),"%s ",fmt->pffmt);
						mprintf(sformat,(octa)*s);
					}
					s++;
				}
			}
			else {
				if(fmt->format == PFMT_STR)
					printChar(value & 0xFF);
				else if(fmt->format == PFMT_FLOAT)
					mprintf("%g",expr.d.floating);
				else {
					msnprintf(sformat,sizeof(sformat),"%s",fmt->pffmt);
					mprintf(sformat,value);
				}
			}
			mprintf("\n");

			// explain special-register
			if(origin == ORG_SREG) {
				const char *explanation = sreg_explain((int)loc,value);
				if(explanation)
					mprintf("%9s%s\n","",explanation);
			}
		}
	}
}

static void printChar(int c) {
	if(isprint(c))
		mprintf("'%c' ",c);
	else
		mprintf("'\\%02u' ",c);
}

static const sPrintFormat *getPrintFormat(int type,const char *fmt) {
	if(!fmt) {
		if(type == ARGVAL_FLOAT)
			return printFormats + 7;
		return printFormats + 0;
	}
	for(size_t i = 1; i < ARRAY_SIZE(printFormats); i++) {
		if(strcmp(printFormats[i].name,fmt) == 0)
			return printFormats + i;
	}
	cmds_throwEx("Unknown format '%s'\n",fmt);
	// not reached
	return NULL;
}
