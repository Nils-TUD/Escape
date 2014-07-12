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

#include <common.h>
#include <spinlock.h>

#define SIG_COUNT			11

#define SIG_IGN				((Signals::handler_func)-2)			/* ignore signal */
#define SIG_ERR				((Signals::handler_func)-1)			/* error-return */
#define SIG_DFL				((Signals::handler_func)0)			/* reset to default behaviour */

/* the signals */
#define SIGRET				-1						/* used to tell the kernel the addr of sigRet */
#define SIGKILL				0
#define SIGABRT				SIGKILL					/* kill process in every case */
#define SIGTERM				1						/* kindly ask process to terminate */
#define SIGFPE				2						/* arithmetic operation (div by zero, ...) */
#define SIGILL				3						/* illegal instruction */
#define SIGINT				4						/* interactive attention signal */
#define SIGSEGV				5						/* segmentation fault */
#define SIGCHLD				6						/* sent to parent-proc */
#define SIGALRM				7						/* for alarm() */
#define SIGUSR1				8						/* can be used for everything */
#define SIGUSR2				9						/* can be used for everything */
#define SIGCANCEL			10						/* is sent by cancel() */

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
	 * @return true if the signal has been sent successfully (i.e. there is a handler or it's fatal)
	 */
	static bool addSignalFor(Thread *t,int signal);

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
	/* we can't add a handler for SIGKILL */
	return signal >= 1 && signal < SIG_COUNT;
}

inline bool Signals::canSend(int signal) {
	return signal == SIGKILL || signal == SIGTERM || signal == SIGINT ||
			signal == SIGUSR1 || signal == SIGUSR2;
}

inline bool Signals::isFatal(int sig) {
	return sig == SIGINT || sig == SIGTERM || sig == SIGFPE || sig == SIGILL ||
		sig == SIGKILL || sig == SIGSEGV;
}
