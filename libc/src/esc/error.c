/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <esc/common.h>
#include <esc/debug.h>
#include <esc/proc.h>
#include <string.h>

#define MAX_STACK_DEPTH 20
/* the x86-call instruction is 5 bytes long */
#define CALL_INSTR_SIZE 5

s32 errno = 0;

u32 *getStackTrace(void) {
	static u32 frames[MAX_STACK_DEPTH];
	u32 i;
	u32 *ebp;
	/* TODO just temporary */
	u32 end = 0xC0000000;
	u32 start = end - 0x1000 * 2;
	u32 *frame = &frames[0];

	GET_REG("ebp",ebp)
	for(i = 0; i < MAX_STACK_DEPTH; i++) {
		/* prevent page-fault */
		if((u32)ebp < start || (u32)ebp >= end)
			break;
		*frame = *(ebp + 1) - CALL_INSTR_SIZE;
		ebp = (u32*)*ebp;
		frame++;
	}

	/* terminate */
	*frame = 0;
	return &frames[0];
}

void printStackTrace(void) {
	u32 *trace = getStackTrace();
	debugf("Stack-trace:\n");
	/* TODO maybe we should skip printStackTrace here? */
	while(*trace != 0) {
		debugf("\t0x%08x\n",*trace);
		trace++;
	}
}

void printLastError(void) {
	debugf("[proc %d] Error %d: %s\n",getpid(),errno,strerror(errno));
	printStackTrace();
}
