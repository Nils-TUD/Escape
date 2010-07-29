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

#ifndef PROC_H_
#define PROC_H_

#include <esc/common.h>

#define MAX_PROC_NAME_LEN	30
#define INVALID_PID			1026

/* the events we can wait for */
#define EV_NOEVENT			0	/* just wakeup on signals */
#define EV_CLIENT			1	/* wakeup when we have a client to be served (for drivers) */
#define EV_RECEIVED_MSG		2	/* wakeup if a driver we're connected to (fd) has a msg for us */
#define EV_DATA_READABLE	8	/* wakeup if we can read from a driver (data available) */

typedef void (*fExitFunc)(void *arg);

typedef struct {
	tPid pid;
	/* the signal that killed the process (SIG_COUNT if none) */
	tSig signal;
	/* exit-code the process gave us via exit() */
	s32 exitCode;
	/* total amount of memory it has used */
	u32 memory;
	/* cycle-count */
	uLongLong ucycleCount;
	uLongLong kcycleCount;
} sExitState;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @return the process-id
 */
tPid getpid(void);

/**
 * @return the parent-pid of the current process
 */
tPid getppid(void);

/**
 * Returns the parent-id of the given process
 *
 * @param pid the process-id
 * @return the parent-pid
 */
s32 getppidof(tPid pid);

/**
 * Clones the current process
 *
 * @return new pid for parent, 0 for child, < 0 if failed
 */
s32 fork(void) A_CHECKRET;

/**
 * Exchanges the process-data with the given program
 *
 * @param path the program-path
 * @param args a NULL-terminated array of arguments
 * @return a negative error-code if failed
 */
s32 exec(const char *path,const char **args);

/**
 * Releases the CPU (reschedule)
 */
void yield(void);

/**
 * Puts the process to sleep for <msecs> milliseconds. If interrupted, ERR_INTERRUPTED is returned
 *
 * @param msecs the number of milliseconds to wait
 * @return 0 on success
 */
s32 sleep(u32 msecs);

/**
 * Puts the process to sleep until one of the given events occurrs. Note that you will
 * always be waked up for signals!
 *
 * @param events the events on which you want to wake up
 * @return a negative error-code if failed
 */
s32 wait(u8 events);

/**
 * Waits until a child terminates and stores information about it into <state>.
 * Note that a child-process is required and only one thread can wait for a child-process!
 * You may get interrupted by a signal (and may want to call waitChild() again in this case). If so
 * you get ERR_INTERRUPTED as return-value (and errno will be set).
 *
 * @param state the exit-state (may be NULL)
 * @return 0 on success
 */
s32 waitChild(sExitState *state);

/**
 * The system function is used to issue a command. Execution of your program will not
 * continue until the command has completed.
 *
 * @param cmd string containing the system command to be executed.
 * @return The value returned when the argument passed is not NULL, depends on the running
 * 	environment specifications. In many systems, 0 is used to indicate that the command was
 * 	successfully executed and other values to indicate some sort of error.
 * 	When the argument passed is NULL, the function returns a nonzero value if the command
 * 	processor is available, and zero otherwise.
 */
s32 system(const char *cmd);

/**
 * Registers the given function for calling when _exit() is called. Registered functions are
 * called in reverse order. Note that the number of functions is limited!
 *
 * @param func the function
 * @return 0 if successfull
 */
s32 atexit(fExitFunc func);

/**
 * The syscall exit
 *
 * @param errorCode the error-code for the parent
 */
void _exit(s32 exitCode) A_NORETURN;

/**
 * Calls atexit-functions and then _exit()
 *
 * @param errorCode the error-code for the parent
 */
void exit(s32 errorCode) A_NORETURN;

#ifdef __cplusplus
}
#endif

#endif /* PROC_H_ */
