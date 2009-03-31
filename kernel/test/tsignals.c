/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <signals.h>
#include <proc.h>
#include "test.h"
#include "tsignals.h"

static void test_signals(void);
static void test_canHandle(void);
static void test_setHandler(void);

/* our test-module */
sTestModule tModSignals = {
	"Signals",
	&test_signals
};

static tSig signals[] = {
	SIG_ILL_INSTR,
	SIG_TERM,
	SIG_INTRPT_ATA1,
	SIG_INTRPT_ATA2,
	SIG_INTRPT_CMOS,
	SIG_INTRPT_COM1,
	SIG_INTRPT_COM2,
	SIG_INTRPT_FLOPPY,
	SIG_INTRPT_KB,
	SIG_INTRPT_TIMER,
	SIG_SEGFAULT
};

static void test_signals(void) {
	test_canHandle();
	test_setHandler();
}

static void test_canHandle(void) {
	u32 i;
	test_caseStart("Testing sig_canHandle()");

	for(i = 0; i < ARRAY_SIZE(signals); i++)
		test_assertTrue(sig_canHandle(signals[i]));
	test_assertFalse(sig_canHandle(SIG_KILL));
	test_assertFalse(sig_canHandle(SIG_COUNT));

	test_caseSucceded();
}

static void test_setHandler(void) {
	sProc *p = proc_getRunning();
	tSig sig = 0xFF;
	tPid pid = 0x1337;
	u32 data = 0;

	test_caseStart("Testing sig_setHandler()");
	test_assertInt(sig_setHandler(p->pid,SIG_INTRPT_ATA1,(fSigHandler)0x123),0);
	test_caseSucceded();

	test_caseStart("Adding and handling a signal");
	test_assertUInt(sig_addSignal(SIG_INTRPT_ATA1,0),p->pid);
	test_assertTrue(sig_hasSignal(&sig,&pid,&data));
	test_assertTrue(pid == p->pid && sig == SIG_INTRPT_ATA1);
	sig_startHandling(pid,sig);
	sig_ackHandling(pid);
	test_assertFalse(sig_hasSignal(&sig,&pid,&data));
	test_caseSucceded();

	test_caseStart("Adding nested signals");
	test_assertUInt(sig_addSignal(SIG_INTRPT_ATA1,0),p->pid);
	test_assertTrue(sig_hasSignal(&sig,&pid,&data));
	test_assertTrue(pid == p->pid && sig == SIG_INTRPT_ATA1);
	sig_startHandling(pid,sig);
	test_assertUInt(sig_addSignal(SIG_INTRPT_ATA1,0),INVALID_PID);
	sig_ackHandling(pid);
	test_assertTrue(sig_hasSignal(&sig,&pid,&data));
	test_assertTrue(pid == p->pid && sig == SIG_INTRPT_ATA1);
	sig_startHandling(pid,sig);
	sig_ackHandling(pid);
	test_assertFalse(sig_hasSignal(&sig,&pid,&data));
	test_caseSucceded();

	test_caseStart("Adding signal for process");
	test_assertInt(sig_setHandler(0x123,SIG_TERM,(fSigHandler)0x456),0);
	test_assertTrue(sig_addSignalFor(0x123,SIG_TERM,0));
	test_assertTrue(sig_hasSignal(&sig,&pid,&data));
	test_assertTrue(pid == 0x123 && sig == SIG_TERM);
	sig_startHandling(pid,sig);
	sig_ackHandling(pid);
	test_assertFalse(sig_hasSignal(&sig,&pid,&data));
	test_caseSucceded();

	test_caseStart("Testing sig_unsetHandler()");
	test_assertUInt(sig_dbg_getHandlerCount(),2);
	sig_unsetHandler(p->pid,SIG_INTRPT_ATA1);
	sig_unsetHandler(0x123,SIG_TERM);
	test_assertUInt(sig_dbg_getHandlerCount(),0);
	test_caseSucceded();

	test_caseStart("Testing sig_removeHandlerFor()");
	sig_setHandler(0x123,SIG_TERM,(fSigHandler)0x456);
	sig_setHandler(0x123,SIG_SEGFAULT,(fSigHandler)0x456);
	sig_setHandler(0x123,SIG_INTRPT_ATA1,(fSigHandler)0x456);
	sig_setHandler(0x456,SIG_INTRPT_ATA1,(fSigHandler)0x1337);
	sig_setHandler(0x456,SIG_INTRPT_COM1,(fSigHandler)0x1337);
	sig_removeHandlerFor(0x123);
	test_assertUInt(sig_dbg_getHandlerCount(),2);
	test_caseSucceded();
}
