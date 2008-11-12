/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/util.h"
#include "../h/video.h"
#include "../h/intrpt.h"
#include "../h/ksymbols.h"
#include <stdarg.h>

/* the x86-call instruction is 5 bytes long */
#define CALL_INSTR_SIZE 5

/**
 * @return the address of the stack-frame-start
 */
extern u32 getStackFrameStart(void);

/**
 * The beginning of the kernel-stack
 */
extern u32 kernelStack;

void panic(cstring fmt,...) {
	va_list ap;
	vid_printf("\n");
	vid_setLineBG(vid_getLine(),RED);
	vid_useColor(RED,WHITE);
	vid_printf("PANIC: ");

	/* print message */
	va_start(ap,fmt);
	vid_vprintf(fmt,ap);
	va_end(ap);

	vid_printf("\n");
	vid_restoreColor();
	printStackTrace();
	intrpt_disable();
	halt();
}

/**
 * Memory address:   Stack elements:
 *
 *                +----------------------------+
 *  0x105000      | Parameter 1 for routine 1  | \
 *                +----------------------------+  |
 *  0x104FFC      | First callers return addr. |   >  Stack frame 1
 *                +----------------------------+  |
 *  0x104FF8      | First callers EBP          | /
 *                +----------------------------+
 *  0x104FF4   +->| Parameter 2 for routine 2  | \  <-- Routine 1's EBP
 *             |  +----------------------------+  |
 *  0x104FF0   |  | Parameter 1 for routine 2  |  |
 *             |  +----------------------------+  |
 *  0x104FEC   |  | Return address, routine 1  |  |
 *             |  +----------------------------+  |
 *  0x104FE8   +--| EBP value for routine 1    |   >  Stack frame 2
 *                +----------------------------+  |
 *  0x104FE4   +->| Local data                 |  | <-- Routine 2's EBP
 *             |  +----------------------------+  |
 *  0x104FE0   |  | Local data                 |  |
 *             |  +----------------------------+  |
 *  0x104FDC   |  | Local data                 | /
 *             |  +----------------------------+
 *  0x104FD8   |  | Parameter 1 for routine 3  | \
 *             |  +----------------------------+  |
 *  0x104FD4   |  | Return address, routine 2  |  |
 *             |  +----------------------------+   >  Stack frame 3
 *  0x104FD0   +--| EBP value for routine 2    |  |
 *                +----------------------------+  |
 *  0x104FCC   +->| Local data                 | /  <-- Routine 3's EBP
 *             |  +----------------------------+
 *  0x104FC8   |  | Return address, routine 3  | \
 *             |  +----------------------------+  |
 *  0x104FC4   +--| EBP value for routine 3    |  |
 *                +----------------------------+   >  Stack frame 4
 *  0x104FC0      | Local data                 |  | <-- Current EBP
 *                +----------------------------+  |
 *  0x104FBC      | Local data                 | /
 *                +----------------------------+
 *  0x104FB8      |                            |    <-- Current ESP
 *                 \/\/\/\/\/\/\/\/\/\/\/\/\/\/
 */
tFuncCall *getStackTrace(void) {
	/* TODO we need a kmalloc here :P */
	static tFuncCall frames[MAX_STACK_DEPTH];
	u32 i;
	tFuncCall *frame = &frames[0];
	tSymbol *sym;
	u32* ebp = (u32*)getStackFrameStart();
	/* TODO we assume here that we always have kernelStack as limit! */
	for(i = 0; i < MAX_STACK_DEPTH && ebp < &kernelStack; i++) {
		frame->addr = *(ebp + 1) - CALL_INSTR_SIZE;
		sym = ksym_getSymbolAt(frame->addr);
		frame->funcAddr = sym->address;
		frame->funcName = sym->funcName;
		ebp = (u32*)*ebp;
		frame++;
	}
	/* terminate */
	frame->addr = 0;
	return &frames[0];
}

void printStackTrace(void) {
	tFuncCall *trace = getStackTrace();
	vid_printf("Stack-trace:\n");
	/* TODO maybe we should skip printStackTrace here? */
	while(trace->addr != 0) {
		vid_printf("\t0x%08x -> 0x%08x (%s)\n",(trace + 1)->addr,trace->funcAddr,trace->funcName);
		trace++;
	}
}

void dumpMem(void *addr,u32 dwordCount) {
	u32 *ptr = (u32*)addr;
	while(dwordCount-- > 0) {
		vid_printf("0x%x: 0x%x\n",ptr,*ptr);
		ptr++;
	}
}

void *memcpy(void *dest,const void *src,size_t len) {
	u8 *d = dest;
	const u8 *s = src;
	while(len--) {
		*d++ = *s++;
	}
	return dest;
}

s32 memcmp(const void *str1,const void *str2,size_t count) {
	const u8 *s1 = str1;
	const u8 *s2 = str2;

	while(count-- > 0) {
		if(*s1++ != *s2++)
			return s1[-1] < s2[-1] ? -1 : 1;
	}
	return 0;
}

void memset(void *addr,u32 value,size_t count) {
	u32 *ptr = (u32*)addr;
	while(count-- > 0) {
		*ptr++ = value;
	}
}
