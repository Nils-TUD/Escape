/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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

#ifndef SIGNAL_H_
#define SIGNAL_H_

#include <esc/common.h>
#include <esc/signals.h>

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

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#endif /* SIGNAL_H_ */
