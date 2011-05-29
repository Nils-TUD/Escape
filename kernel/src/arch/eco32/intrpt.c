/**
 * $Id$
 */

#include <esc/common.h>
#include <sys/intrpt.h>
#include <sys/video.h>
#include <sys/util.h>

typedef void (*fIntrptHandler)(sIntrptStackFrame *frame);

/**
 * Handles a TLB-miss >= 0x80000000 && < 0xC0000000
 */
extern void intrpt_kernelMiss(sIntrptStackFrame *frame);
/**
 * The default handler for all interrupts we do not expect
 */
static void intrpt_defHandler(sIntrptStackFrame *frame);

static const char *irq2Name[] = {
	/*  0 */ "Terminal 0 Transmitter",
	/*  1 */ "Terminal 0 Receiver",
	/*  2 */ "Terminal 1 Transmitter",
	/*  3 */ "Terminal 1 Receiver",
	/*  4 */ "Keyboard",
	/*  5 */ "Unused",
	/*  6 */ "Unused",
	/*  7 */ "Unused",
	/*  8 */ "Disk",
	/*  9 */ "Unused",
	/* 10 */ "Unused",
	/* 11 */ "Unused",
	/* 12 */ "Unused",
	/* 13 */ "Unused",
	/* 14 */ "Timer",
	/* 15 */ "Unused",
	/* 16 */ "Bus timeout",
	/* 17 */ "Illegal instruction",
	/* 18 */ "Privileged instruction",
	/* 19 */ "Divide instruction",
	/* 20 */ "Trap instruction",
	/* 21 */ "TLB miss",
	/* 22 */ "TLB write",
	/* 23 */ "TLB invalid",
	/* 24 */ "Illegal address",
	/* 25 */ "Privileged address",
	/* 26 */ "Unused",
	/* 27 */ "Unused",
	/* 28 */ "Unused",
	/* 29 */ "Unused",
	/* 30 */ "Unused",
	/* 31 */ "Unused",
};

static fIntrptHandler handler[32] = {
	/*  0 */ intrpt_defHandler,						/* terminal 0 transmitter interrupt */
	/*  1 */ intrpt_defHandler,						/* terminal 0 receiver interrupt */
	/*  2 */ intrpt_defHandler,						/* terminal 1 transmitter interrupt */
	/*  3 */ intrpt_defHandler,						/* terminal 1 receiver interrupt */
	/*  4 */ intrpt_defHandler,						/* keyboard interrupt */
	/*  5 */ intrpt_defHandler,
	/*  6 */ intrpt_defHandler,
	/*  7 */ intrpt_defHandler,
	/*  8 */ intrpt_defHandler,						/* disk interrupt */
	/*  9 */ intrpt_defHandler,
	/* 10 */ intrpt_defHandler,
	/* 11 */ intrpt_defHandler,
	/* 12 */ intrpt_defHandler,
	/* 13 */ intrpt_defHandler,
	/* 14 */ intrpt_defHandler,						/* timer interrupt */
	/* 15 */ intrpt_defHandler,
	/* 16 */ intrpt_defHandler,						/* bus timeout exception */
	/* 17 */ intrpt_defHandler,						/* illegal instruction exception */
	/* 18 */ intrpt_defHandler,						/* privileged instruction exception */
	/* 19 */ intrpt_defHandler,						/* divide instruction exception */
	/* 20 */ intrpt_defHandler,						/* trap instruction exception */
	/* 21 */ intrpt_kernelMiss,						/* TLB miss exception */
	/* 22 */ intrpt_defHandler,						/* TLB write exception */
	/* 23 */ intrpt_defHandler,						/* TLB invalid exception */
	/* 24 */ intrpt_defHandler,						/* illegal address exception */
	/* 25 */ intrpt_defHandler,						/* privileged address exception */
	/* 26 */ intrpt_defHandler,
	/* 27 */ intrpt_defHandler,
	/* 28 */ intrpt_defHandler,
	/* 29 */ intrpt_defHandler,
	/* 30 */ intrpt_defHandler,
	/* 31 */ intrpt_defHandler,
};
static size_t irqCount = 0;
static sIntrptStackFrame *curFrame;

void intrpt_handler(sIntrptStackFrame *frame) {
	curFrame = frame;
	irqCount++;

	/* call handler */
	handler[frame->irqNo & 0x1f](frame);
}

size_t intrpt_getCount(void) {
	return irqCount;
}

sIntrptStackFrame *intrpt_getCurStack(void) {
	return curFrame;
}

static void intrpt_defHandler(sIntrptStackFrame *frame) {
	/* do nothing */
	util_panic("Got interrupt %d (%s) @ 0x%08x\n",
			frame->irqNo,irq2Name[frame->irqNo & 0x1f],frame->r[30]);
}
