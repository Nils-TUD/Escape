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
#include <assert.h>
#include "esc/test.h"
#include "tsignals.h"

static void test_signals();
static void test_canHandle();
static void test_setHandler();

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

static void test_signals() {
	test_canHandle();
	test_setHandler();
}

static void test_canHandle() {
	test_caseStart("Testing Signals::canHandle()");

	for(size_t i = 0; i < ARRAY_SIZE(signals); i++)
		test_assertTrue(Signals::canHandle(signals[i]));
	test_assertFalse(Signals::canHandle(SIG_KILL));
	test_assertFalse(Signals::canHandle(SIG_COUNT));

	test_caseSucceeded();
}

static void test_setHandler() {
	Thread *t1 = Thread::getRunning();
	int tid = Proc::startThread(0,0,NULL);
	Thread *t2 = Thread::getById(tid);
	int sig = 0xFF;
	Signals::handler_func handler,old;

	test_caseStart("Testing Signals::setHandler()");
	test_assertInt(Signals::setHandler(t2->getTid(),SIG_INTRPT_ATA1,(Signals::handler_func)0x123,&old),0);
	test_caseSucceeded();

	test_caseStart("Adding a signal for somebody else");
	test_assertTrue(Signals::addSignal(SIG_INTRPT_ATA1));
	test_assertTrue(Signals::hasSignalFor(t2->getTid()));
	test_caseSucceeded();

	test_caseStart("Adding a signal for us");
	test_assertInt(Signals::setHandler(t1->getTid(),SIG_INTRPT_ATA2,(Signals::handler_func)0x456,&old),0);
	test_assertTrue(Signals::addSignal(SIG_INTRPT_ATA2));
	test_assertTrue(Signals::hasSignalFor(t1->getTid()));
	test_assertTrue(Signals::checkAndStart(t1->getTid(),&sig,&handler));
	test_assertTrue(sig == SIG_INTRPT_ATA2);
	test_assertTrue(handler == (Signals::handler_func)0x456);
	test_assertTrue(Signals::ackHandling(t1->getTid()) == sig);
	test_assertFalse(Signals::hasSignalFor(t1->getTid()));
	test_assertTrue(Signals::unsetHandler(t1->getTid(),SIG_INTRPT_ATA2) == (Signals::handler_func)0x456);
	test_caseSucceeded();

	test_caseStart("Adding signal for own process");
	test_assertInt(Signals::setHandler(t1->getTid(),SIG_TERM,(Signals::handler_func)0x456,&old),0);
	Proc::addSignalFor(t1->getProc()->getPid(),SIG_TERM);
	test_assertTrue(Signals::hasSignalFor(t1->getTid()));
	test_assertTrue(Signals::checkAndStart(t1->getTid(),&sig,&handler));
	test_assertTrue(sig == SIG_TERM);
	test_assertTrue(handler == (Signals::handler_func)0x456);
	test_assertTrue(Signals::ackHandling(t1->getTid()) == sig);
	test_assertFalse(Signals::hasSignalFor(t1->getTid()));
	test_caseSucceeded();

	test_caseStart("Adding nested signals");
	test_assertTrue(Signals::addSignal(SIG_TERM));
	test_assertTrue(Signals::hasSignalFor(t1->getTid()));
	test_assertTrue(Signals::checkAndStart(t1->getTid(),&sig,&handler));
	test_assertTrue(sig == SIG_TERM);
	test_assertTrue(handler == (Signals::handler_func)0x456);
	test_assertTrue(Signals::addSignal(SIG_TERM));
	test_assertTrue(Signals::ackHandling(t1->getTid()) == SIG_TERM);
	test_assertTrue(Signals::hasSignalFor(t1->getTid()));
	test_assertTrue(Signals::checkAndStart(t1->getTid(),&sig,&handler));
	test_assertTrue(sig == SIG_TERM);
	test_assertTrue(handler == (Signals::handler_func)0x456);
	test_assertTrue(Signals::ackHandling(t1->getTid()) == SIG_TERM);
	test_assertFalse(Signals::hasSignalFor(t1->getTid()));
	test_caseSucceeded();

	test_caseStart("Testing Signals::unsetHandler()");
	test_assertSize(Signals::getHandlerCount(),2);
	Signals::unsetHandler(t2->getTid(),SIG_INTRPT_ATA1);
	Signals::unsetHandler(t1->getTid(),SIG_TERM);
	test_assertSize(Signals::getHandlerCount(),0);
	test_caseSucceeded();

	test_caseStart("Testing Signals::unsetHandler() with pending signals");
	test_assertInt(Signals::setHandler(t1->getTid(),SIG_TERM,(Signals::handler_func)0x456,&old),0);
	test_assertTrue(Signals::addSignal(SIG_TERM));
	test_assertTrue(Signals::addSignal(SIG_TERM));
	test_assertTrue(Signals::hasSignalFor(t1->getTid()));
	Signals::unsetHandler(t1->getTid(),SIG_TERM);
	test_assertFalse(Signals::hasSignalFor(t1->getTid()));
	/* try again */
	test_assertInt(Signals::setHandler(t1->getTid(),SIG_TERM,(Signals::handler_func)0x456,&old),0);
	test_assertTrue(Signals::addSignal(SIG_TERM));
	test_assertTrue(Signals::addSignal(SIG_TERM));
	test_assertTrue(Signals::hasSignalFor(t1->getTid()));
	Signals::unsetHandler(t1->getTid(),SIG_TERM);
	test_assertFalse(Signals::hasSignalFor(t1->getTid()));
	/* try again with checkAndStart( */
	test_assertInt(Signals::setHandler(t1->getTid(),SIG_TERM,(Signals::handler_func)0x456,&old),0);
	test_assertTrue(Signals::addSignal(SIG_TERM));
	test_assertTrue(Signals::addSignal(SIG_TERM));
	test_assertTrue(Signals::hasSignalFor(t1->getTid()));
	Signals::unsetHandler(t1->getTid(),SIG_TERM);
	test_assertFalse(Signals::hasSignalFor(t1->getTid()));
	/* try again */
	test_assertInt(Signals::setHandler(t1->getTid(),SIG_TERM,(Signals::handler_func)0x456,&old),0);
	test_assertTrue(Signals::addSignal(SIG_TERM));
	test_assertTrue(Signals::addSignal(SIG_TERM));
	test_assertTrue(Signals::hasSignalFor(t1->getTid()));
	Signals::unsetHandler(t1->getTid(),SIG_TERM);
	test_assertFalse(Signals::hasSignalFor(t1->getTid()));
	test_caseSucceeded();

	test_caseStart("Testing Signals::removeHandlerFor)");
	test_assertInt(Signals::setHandler(t1->getTid(),SIG_TERM,(Signals::handler_func)0x456,&old),0);
	test_assertInt(Signals::setHandler(t1->getTid(),SIG_SEGFAULT,(Signals::handler_func)0x456,&old),0);
	test_assertInt(Signals::setHandler(t1->getTid(),SIG_INTRPT_ATA1,(Signals::handler_func)0x456,&old),0);
	test_assertInt(Signals::setHandler(t2->getTid(),SIG_INTRPT_ATA1,(Signals::handler_func)0x1337,&old),0);
	test_assertInt(Signals::setHandler(t2->getTid(),SIG_INTRPT_COM1,(Signals::handler_func)0x1337,&old),0);
	Signals::removeHandlerFor(t2->getTid());
	test_assertSize(Signals::getHandlerCount(),3);
	Signals::removeHandlerFor(t1->getTid());
	test_assertSize(Signals::getHandlerCount(),0);
	test_caseSucceeded();

	assert(t2->beginTerm());
	Proc::killThread(tid);
}
