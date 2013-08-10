/**
 * $Id: exception.c 160 2011-04-09 16:58:24Z nasmussen $
 */

#include <setjmp.h>
#include <string.h>
#include <assert.h>

#include "common.h"
#include "exception.h"
#include "core/register.h"
#include "mmix/error.h"

#define MAX_EXNAME_LEN		256

jmp_buf *envs[MAX_ENV_NEST_DEPTH];
int curEnv = -1;
int curEx = 0;
octa curBits = 0;

static const char *exNames[] = {
	"No exception",
	"Forced trip",
	"Dynamic trip",
	"Forced trap",
	"Dynamic trap",
	"A CLI exception",
	"Breakpoint exception",
};
static const char *trapNames[] = {
	/* 00 */ "Power failure",
	/* 01 */ "Memory parity error",
	/* 02 */ "Nonexistent memory",
	/* 03 */ "Reboot",
	/* 04 */ "???",
	/* 05 */ "???",
	/* 06 */ "???",
	/* 07 */ "Interval counter",
	/* 08 */ "???",
	/* 09 */ "???",
	/* 0A */ "???",
	/* 0B */ "???",
	/* 0C */ "???",
	/* 0D */ "???",
	/* 0E */ "???",
	/* 0F */ "???",
	/* 10 */ "???",
	/* 11 */ "???",
	/* 12 */ "???",
	/* 13 */ "???",
	/* 14 */ "???",
	/* 15 */ "???",
	/* 16 */ "???",
	/* 17 */ "???",
	/* 18 */ "???",
	/* 19 */ "???",
	/* 1A */ "???",
	/* 1B */ "???",
	/* 1C */ "???",
	/* 1D */ "???",
	/* 1E */ "Software translation",
	/* 1F */ "Repeat instruction",
	/* 20 */ "Privileged PC",
	/* 21 */ "Security violation",
	/* 22 */ "Breaks rules of MMIX",
	/* 23 */ "Privileged instruction",
	/* 24 */ "Privileged access",
	/* 25 */ "Execution protection",
	/* 26 */ "Write protection",
	/* 27 */ "Read protection",
	/* 28 */ "???",
	/* 29 */ "???",
	/* 2A */ "???",
	/* 2B */ "???",
	/* 2C */ "???",
	/* 2D */ "???",
	/* 2E */ "???",
	/* 2F */ "???",
	/* 30 */ "???",
	/* 31 */ "???",
	/* 32 */ "???",
	/* 33 */ "Disk",
	/* 34 */ "Timer",
	/* 35 */ "Term0 receiver",
	/* 36 */ "Term0 transmitter",
	/* 37 */ "Term1 receiver",
	/* 38 */ "Term1 transmitter",
	/* 39 */ "Term2 receiver",
	/* 3A */ "Term2 transmitter",
	/* 3B */ "Term3 receiver",
	/* 3C */ "Term3 transmitter",
	/* 3D */ "Keyboard",
	/* 3E */ "???",
	/* 3F */ "???",
};
static const char *tripNames[] = {
	"Float inexact",
	"Float division by zero",
	"Float underflow",
	"Float overflow",
	"Float invalid operation",
	"Float to fix overflow",
	"Integer overflow",
	"Integer division by zero",
};

const char *ex_toString(int exception,octa bits) {
	static char buffer[MAX_EXNAME_LEN];
	assert(exception >= 0 && exception < (int)ARRAY_SIZE(exNames));
	strcpy(buffer,exNames[exception]);
	if(exception == EX_DYNAMIC_TRAP || exception == EX_DYNAMIC_TRIP) {
		const char **names = exception == EX_DYNAMIC_TRAP ? trapNames : tripNames;
		// the function is for debugging. therefore we don't care much about speed ;)
		strcat(buffer,": ");
		for(int k = 0; bits; k++) {
			if(bits & ((octa)1 << k)) {
				bits &= ~((octa)1 << k);
				strcat(buffer,names[k]);
				if(bits)
					strcat(buffer,", ");
			}
		}
	}
	return buffer;
}

void ex_throw(int exception,octa bits) {
	assert(curEnv >= 0);
	curEx = exception;
	curBits = bits;
	longjmp(*envs[curEnv],exception);
}

void ex_rethrow(void) {
	assert(curEx != EX_NONE);
	ex_throw(curEx,curBits);
}
