/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#pragma once

#include <sys/common.h>
#include <sys/spinlock.h>

#define SIG_COUNT			9

#define SIG_IGN				((Signals::handler_func)-2)			/* ignore signal */
#define SIG_ERR				((Signals::handler_func)-1)			/* error-return */
#define SIG_DFL				((Signals::handler_func)0)			/* reset to default behaviour */

/* the signals */
#define SIG_RET				-1						/* used to tell the kernel the addr of sigRet */
#define SIG_KILL			0						/* kills a proc; not catchable */
#define SIG_TERM			1						/* terminates a proc; catchable */
#define SIG_ILL_INSTR		2						/* TODO atm unused */
#define SIG_SEGFAULT		3						/* segmentation fault */
#define SIG_CHILD_TERM		4						/* sent to parent-proc */
#define SIG_INTRPT			5						/* used to interrupt a process; used by shell */
#define SIG_ALARM			6						/* for alarm() */
#define SIG_USR1			7						/* can be used for everything */
#define SIG_USR2			8						/* can be used for everything */

class Thread;
class ThreadBase;
class OStream;

class Signals {
	friend class ThreadBase;

	Signals() = delete;

public:
	/* signal-handler-signature */
	typedef void (*handler_func)(int);

	/**
	 * Checks whether we can handle the given signal
	 *
	 * @param signal the signal
	 * @return true if so
	 */
	static bool canHandle(int signal);

	/**
	 * Checks whether the given signal can be send by user-programs
	 *
	 * @param signal the signal
	 * @return true if so
	 */
	static bool canSend(int signal);

	/**
	 * @param sig the signal
	 * @return whether the given signal is a fatal one, i.e. should kill the process
	 */
	static bool isFatal(int sig);

	/**
	 * Sets the given signal-handler for <signal>
	 *
	 * @param t the current thread
	 * @param signal the signal
	 * @param func the handler-function
	 * @param old will be set to the old handler
	 * @return 0 on success
	 */
	static handler_func setHandler(Thread *t,int signal,handler_func func);

	/**
	 * Checks whether the current thread should handle a signal. If so, it sets *sig and *handler
	 * correspondingly.
	 *
	 * @param t the current thread
	 * @param sig will be set to the signal to handle, if the current thread has a signal
	 * @param handler will be set to the handler, if the current thread has a signal
	 * @return true if there is a signal to handle
	 */
	static bool checkAndStart(Thread *t,int *sig,handler_func *handler);

	/**
	 * Adds the given signal for the given thread
	 *
	 * @param t the thread
	 * @param signal the signal
	 */
	static void addSignalFor(Thread *t,int signal);

	/**
	 * @param signal the signal-number
	 * @return the name of the given signal
	 */
	static const char *getName(int signal);

	/**
	 * Prints the signal-handlers of thread t.
	 *
	 * @param t the thread
	 * @param os the output-stream
	 */
	static void print(const Thread *t,OStream &os);
};

inline bool Signals::canHandle(int signal) {
	/* we can't add a handler for SIG_KILL */
	return signal >= 1 && signal < SIG_COUNT;
}

inline bool Signals::canSend(int signal) {
	return signal == SIG_KILL || signal == SIG_TERM || signal == SIG_INTRPT ||
			signal == SIG_USR1 || signal == SIG_USR2;
}

inline bool Signals::isFatal(int sig) {
	return sig == SIG_INTRPT || sig == SIG_TERM || sig == SIG_KILL || sig == SIG_SEGFAULT;
}
