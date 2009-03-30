/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef SIGNAL_H_
#define SIGNAL_H_

#include "../../esc/h/common.h"
#include "../../esc/h/signals.h"

#define SIGABRT		SIG_KILL
/* TODO */
#define SIGFPE		-1
#define SIGILL		SIG_ILL_INSTR
#define SIGINT		SIG_INTRPT
#define SIGSEGV		SIG_SEGFAULT
#define SIGTERM		SIG_TERM
/* TODO */
#define SIG_IGN		-1
#define SIG_DFL		-1

/**
 * The  signal  system call installs a new signal handler for#
 * signal signum.  The signal handler is set to handler which
 * may be a user specified function, or one of the following:
 * 	SIG_IGN - Ignore the signal.
 * 	SIG_DFL - Reset the signal to its default behavior.
 *
 * @param sig the signal
 * @param handler the new handler-function
 * @return the previous handler-function
 */
void (*signal(int sig,void (*handler)(int)))(int);

/**
 * raise()  sends  a  signal  to  the current process.  It is
 * equivalent to kill(getpid(),sig)
 *
 * @param sig the signal
 * @return zero on success
 */
int raise(int sig);

#endif /* SIGNAL_H_ */
