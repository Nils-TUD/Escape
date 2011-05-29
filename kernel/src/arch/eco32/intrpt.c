/**
 * $Id$
 */

#include <esc/common.h>
#include <sys/task/signals.h>
#include <sys/task/timer.h>
#include <sys/syscalls.h>
#include <sys/intrpt.h>
#include <sys/video.h>
#include <sys/util.h>

/* interrupt- and exception-numbers */
#define IRQ_TTY0_XMTR		0
#define IRQ_TTY0_RCVR		1
#define IRQ_TTY1_XMTR		2
#define IRQ_TTY1_RCVR		3
#define IRQ_KEYBOARD		4
#define IRQ_DISK			8
#define IRQ_TIMER			14
#define EXC_BUS_TIMEOUT		16
#define EXC_ILL_INSTRCT		17
#define EXC_PRV_INSTRCT		18
#define EXC_DIVIDE			19
#define EXC_TRAP			20
#define EXC_TLB_MISS		21
#define EXC_TLB_WRITE		22
#define EXC_TLB_INVALID		23
#define EXC_ILL_ADDRESS		24
#define EXC_PRV_ADDRESS		25

typedef void (*fIntrptHandler)(sIntrptStackFrame *frame);
typedef struct {
	fIntrptHandler handler;
	const char *name;
	tSig signal;
} sInterrupt;

extern void intrpt_exKMiss(sIntrptStackFrame *frame);
static void intrpt_defHandler(sIntrptStackFrame *frame);
static void intrpt_exTrap(sIntrptStackFrame *frame);
static void intrpt_irqTimer(sIntrptStackFrame *stack);

static sInterrupt intrptList[] = {
	/* 0x00: IRQ_TTY0_XMTR */	{intrpt_defHandler,	"Terminal 0 Transmitter",0},
	/* 0x01: IRQ_TTY0_RCVR */	{intrpt_defHandler,	"Terminal 0 Receiver",	0},
	/* 0x02: IRQ_TTY1_XMTR */	{intrpt_defHandler,	"Terminal 1 Transmitter",0},
	/* 0x03: IRQ_TTY1_RCVR */	{intrpt_defHandler,	"Terminal 1 Receiver",	0},
	/* 0x04: IRQ_KEYBOARD */	{intrpt_defHandler,	"Keyboard",				SIG_INTRPT_KB},
	/* 0x05: -- */				{intrpt_defHandler,	"??",					0},
	/* 0x06: -- */				{intrpt_defHandler,	"??",					0},
	/* 0x07: -- */				{intrpt_defHandler,	"??",					0},
	/* 0x08: IRQ_DISK */		{intrpt_defHandler,	"Disk",					SIG_INTRPT_ATA1},
	/* 0x09: -- */				{intrpt_defHandler,	"??",					0},
	/* 0x0A: - */				{intrpt_defHandler,	"??",					0},
	/* 0x0B: -- */				{intrpt_defHandler,	"??",					0},
	/* 0x0C: -- */				{intrpt_defHandler,	"??",					0},
	/* 0x0D: -- */				{intrpt_defHandler,	"??",					0},
	/* 0x0E: IRQ_TIMER */		{intrpt_irqTimer,	"Timer",				SIG_INTRPT_TIMER},
	/* 0x0F: -- */				{intrpt_defHandler,	"??",					0},
	/* 0x10: EXC_BUS_TIMEOUT */	{intrpt_defHandler,	"Bus timeout exception",0},
	/* 0x11: EXC_ILL_INSTRCT */	{intrpt_defHandler,	"Ill. instr. exception",0},
	/* 0x12: EXC_PRV_INSTRCT */	{intrpt_defHandler,	"Prv. instr. exception",0},
	/* 0x13: EXC_DIVIDE */		{intrpt_defHandler,	"Divide exception",		0},
	/* 0x14: EXC_TRAP */		{intrpt_exTrap,		"Trap exception",		0},
	/* 0x15: EXC_TLB_MISS */	{intrpt_exKMiss,	"TLB miss exception",	0},
	/* 0x16: EXC_TLB_WRITE */	{intrpt_defHandler,	"TLB write exception",	0},
	/* 0x17: EXC_TLB_INVALID */	{intrpt_defHandler,	"TLB invalid exception",0},
	/* 0x18: EXC_ILL_ADDRESS */	{intrpt_defHandler,	"Ill. addr. exception",	0},
	/* 0x19: EXC_PRV_ADDRESS */	{intrpt_defHandler,	"Prv. addr. exception",	0},
	/* 0x1A: -- */				{intrpt_defHandler,	"??",					0},
	/* 0x1B: -- */				{intrpt_defHandler,	"??",					0},
	/* 0x1C: -- */				{intrpt_defHandler,	"??",					0},
	/* 0x1D: -- */				{intrpt_defHandler,	"??",					0},
	/* 0x1E: -- */				{intrpt_defHandler,	"??",					0},
	/* 0x1F: -- */				{intrpt_defHandler,	"??",					0},
};
static size_t irqCount = 0;
static sIntrptStackFrame *curFrame;

void intrpt_handler(sIntrptStackFrame *frame) {
	curFrame = frame;
	irqCount++;

	/* call handler */
	intrptList[frame->irqNo & 0x1f].handler(frame);
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
			frame->irqNo,intrptList[frame->irqNo & 0x1f].name,frame->r[30]);
}

static void intrpt_exTrap(sIntrptStackFrame *frame) {
	cons_start();
	sThread *t = thread_getRunning();
	t->stats.syscalls++;
	sysc_handle(frame);
}

static void intrpt_irqTimer(sIntrptStackFrame *stack) {
	sInterrupt *intrpt = intrptList + stack->irqNo;
	if(intrpt->signal)
		sig_addSignal(intrpt->signal);
	timer_intrpt();
	timer_ackIntrpt();
}
