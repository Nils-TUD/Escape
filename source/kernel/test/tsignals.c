/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <sys/common.h>
#include <sys/task/signals.h>
#include <sys/task/proc.h>
#include <sys/task/thread.h>
#include "esc/test.h"
#include "tsignals.h"

static void test_signals(void);
static void test_canHandle(void);
static void test_setHandler(void);

/* our test-module */
sTestModule tModSignals = {
	"Signals",
	&test_signals
};

static int signals[] = {
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
	size_t i;
	test_caseStart("Testing sig_canHandle()");

	for(i = 0; i < ARRAY_SIZE(signals); i++)
		test_assertTrue(sig_canHandle(signals[i]));
	test_assertFalse(sig_canHandle(SIG_KILL));
	test_assertFalse(sig_canHandle(SIG_COUNT));

	test_caseSucceeded();
}

static void test_setHandler(void) {
	sThread *t1 = thread_getRunning();
	int tid = proc_startThread(0,0,NULL);
	sThread *t2 = thread_getById(tid);
	int sig = 0xFF;
	fSignal handler,old;

	test_caseStart("Testing sig_setHandler()");
	test_assertInt(sig_setHandler(t2->tid,SIG_INTRPT_ATA1,(fSignal)0x123,&old),0);
	test_caseSucceeded();

	test_caseStart("Adding and handling a signal");
	test_assertTrue(sig_addSignal(SIG_INTRPT_ATA1));
	test_assertTrue(sig_hasSignalFor(t2->tid));
	test_assertInt(sig_checkAndStart(t2->tid,&sig,&handler),SIG_CHECK_CUR);
	test_assertTrue(sig == SIG_INTRPT_ATA1);
	test_assertTrue(handler == (fSignal)0x123);
	test_assertTrue(sig_ackHandling(t2->tid) == SIG_INTRPT_ATA1);
	test_assertFalse(sig_hasSignalFor(t2->tid));
	test_caseSucceeded();

	test_caseStart("Adding nested signals");
	test_assertTrue(sig_addSignal(SIG_INTRPT_ATA1));
	test_assertTrue(sig_hasSignalFor(t2->tid));
	test_assertInt(sig_checkAndStart(t2->tid,&sig,&handler),SIG_CHECK_CUR);
	test_assertTrue(sig == SIG_INTRPT_ATA1);
	test_assertTrue(handler == (fSignal)0x123);
	test_assertTrue(sig_addSignal(SIG_INTRPT_ATA1));
	test_assertTrue(sig_ackHandling(t2->tid) == SIG_INTRPT_ATA1);
	test_assertTrue(sig_hasSignalFor(t2->tid));
	test_assertInt(sig_checkAndStart(t2->tid,&sig,&handler),SIG_CHECK_CUR);
	test_assertTrue(sig == SIG_INTRPT_ATA1);
	test_assertTrue(handler == (fSignal)0x123);
	test_assertTrue(sig_ackHandling(t2->tid) == SIG_INTRPT_ATA1);
	test_assertFalse(sig_hasSignalFor(t2->tid));
	test_caseSucceeded();

	test_caseStart("Adding signal for process");
	test_assertInt(sig_setHandler(t1->tid,SIG_TERM,(fSignal)0x456,&old),0);
	proc_addSignalFor(t1->proc->pid,SIG_TERM);
	test_assertTrue(sig_hasSignalFor(t1->tid));
	test_assertInt(sig_checkAndStart(t1->tid,&sig,&handler),SIG_CHECK_CUR);
	test_assertTrue(sig == SIG_TERM);
	test_assertTrue(handler == (fSignal)0x456);
	test_assertTrue(sig_ackHandling(t1->tid) == sig);
	test_assertFalse(sig_hasSignalFor(t1->tid));
	test_caseSucceeded();

	test_caseStart("Testing sig_unsetHandler()");
	test_assertSize(sig_dbg_getHandlerCount(),2);
	sig_unsetHandler(t2->tid,SIG_INTRPT_ATA1);
	sig_unsetHandler(t1->tid,SIG_TERM);
	test_assertSize(sig_dbg_getHandlerCount(),0);
	test_caseSucceeded();

	test_caseStart("Testing sig_unsetHandler() with pending signals");
	test_assertInt(sig_setHandler(t1->tid,SIG_TERM,(fSignal)0x456,&old),0);
	test_assertTrue(sig_addSignal(SIG_TERM));
	test_assertTrue(sig_addSignal(SIG_TERM));
	test_assertTrue(sig_hasSignalFor(t1->tid));
	sig_unsetHandler(t1->tid,SIG_TERM);
	test_assertFalse(sig_hasSignalFor(t1->tid));
	/* try again */
	test_assertInt(sig_setHandler(t1->tid,SIG_TERM,(fSignal)0x456,&old),0);
	test_assertTrue(sig_addSignal(SIG_TERM));
	test_assertTrue(sig_addSignal(SIG_TERM));
	test_assertTrue(sig_hasSignalFor(t1->tid));
	sig_unsetHandler(t1->tid,SIG_TERM);
	test_assertFalse(sig_hasSignalFor(t1->tid));
	/* try again with checkAndStart */
	test_assertInt(sig_setHandler(t1->tid,SIG_TERM,(fSignal)0x456,&old),0);
	test_assertTrue(sig_addSignal(SIG_TERM));
	test_assertTrue(sig_addSignal(SIG_TERM));
	test_assertInt(sig_checkAndStart(t2->tid,&sig,&handler),SIG_CHECK_OTHER);
	test_assertTrue(sig_hasSignalFor(t1->tid));
	sig_unsetHandler(t1->tid,SIG_TERM);
	test_assertFalse(sig_hasSignalFor(t1->tid));
	/* try again */
	test_assertInt(sig_setHandler(t1->tid,SIG_TERM,(fSignal)0x456,&old),0);
	test_assertTrue(sig_addSignal(SIG_TERM));
	test_assertTrue(sig_addSignal(SIG_TERM));
	test_assertTrue(sig_hasSignalFor(t1->tid));
	sig_unsetHandler(t1->tid,SIG_TERM);
	test_assertFalse(sig_hasSignalFor(t1->tid));
	test_caseSucceeded();

	test_caseStart("Testing sig_removeHandlerFor()");
	test_assertInt(sig_setHandler(t1->tid,SIG_TERM,(fSignal)0x456,&old),0);
	test_assertInt(sig_setHandler(t1->tid,SIG_SEGFAULT,(fSignal)0x456,&old),0);
	test_assertInt(sig_setHandler(t1->tid,SIG_INTRPT_ATA1,(fSignal)0x456,&old),0);
	test_assertInt(sig_setHandler(t2->tid,SIG_INTRPT_ATA1,(fSignal)0x1337,&old),0);
	test_assertInt(sig_setHandler(t2->tid,SIG_INTRPT_COM1,(fSignal)0x1337,&old),0);
	sig_removeHandlerFor(t2->tid);
	test_assertSize(sig_dbg_getHandlerCount(),3);
	sig_removeHandlerFor(t1->tid);
	test_assertSize(sig_dbg_getHandlerCount(),0);
	test_caseSucceeded();

	proc_killThread(tid);
}
