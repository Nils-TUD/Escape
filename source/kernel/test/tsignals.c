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

static sig_t signals[] = {
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
	sThread *t = thread_getRunning();
	sig_t sig = 0xFF;
	pid_t tid = 0x1337;

	test_caseStart("Testing sig_setHandler()");
	sig_setHandler(t->tid,SIG_INTRPT_ATA1,(fSignal)0x123);
	test_caseSucceeded();

	test_caseStart("Adding and handling a signal");
	sig_addSignal(SIG_INTRPT_ATA1);
	test_assertTrue(sig_hasSignal(&sig,&tid));
	test_assertTrue(tid == t->tid && sig == SIG_INTRPT_ATA1);
	sig_startHandling(tid,sig);
	sig_ackHandling(tid);
	test_assertFalse(sig_hasSignal(&sig,&tid));
	test_caseSucceeded();

	test_caseStart("Adding nested signals");
	sig_addSignal(SIG_INTRPT_ATA1);
	test_assertTrue(sig_hasSignal(&sig,&tid));
	test_assertTrue(tid == t->tid && sig == SIG_INTRPT_ATA1);
	sig_startHandling(tid,sig);
	sig_addSignal(SIG_INTRPT_ATA1);
	sig_ackHandling(tid);
	test_assertTrue(sig_hasSignal(&sig,&tid));
	test_assertTrue(tid == t->tid && sig == SIG_INTRPT_ATA1);
	sig_startHandling(tid,sig);
	sig_ackHandling(tid);
	test_assertFalse(sig_hasSignal(&sig,&tid));
	test_caseSucceeded();

	test_caseStart("Adding signal for process");
	sig_setHandler(t->tid,SIG_TERM,(fSignal)0x456);
	sig_addSignalFor(t->proc->pid,SIG_TERM);
	test_assertTrue(sig_hasSignal(&sig,&tid));
	test_assertTrue(tid == t->tid && sig == SIG_TERM);
	sig_startHandling(tid,sig);
	sig_ackHandling(tid);
	test_assertFalse(sig_hasSignal(&sig,&tid));
	test_caseSucceeded();

	test_caseStart("Testing sig_unsetHandler()");
	test_assertSize(sig_dbg_getHandlerCount(),2);
	sig_unsetHandler(t->tid,SIG_INTRPT_ATA1);
	sig_unsetHandler(t->tid,SIG_TERM);
	test_assertSize(sig_dbg_getHandlerCount(),0);
	test_caseSucceeded();

	test_caseStart("Testing sig_removeHandlerFor()");
	sig_setHandler(0,SIG_TERM,(fSignal)0x456);
	sig_setHandler(0,SIG_SEGFAULT,(fSignal)0x456);
	sig_setHandler(0,SIG_INTRPT_ATA1,(fSignal)0x456);
	test_assertTrue(thread_getById(1) != NULL);
	if(thread_getById(1) != NULL) {
		sig_setHandler(1,SIG_INTRPT_ATA1,(fSignal)0x1337);
		sig_setHandler(1,SIG_INTRPT_COM1,(fSignal)0x1337);
		sig_removeHandlerFor(1);
	}
	test_assertSize(sig_dbg_getHandlerCount(),3);
	test_caseSucceeded();
}
