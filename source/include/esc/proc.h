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

typedef void (*fExitFunc)(void *arg);

typedef struct {
	pid_t pid;
	/* the signal that killed the process (SIG_COUNT if none) */
	sig_t signal;
	/* exit-code the process gave us via exit() */
	int exitCode;
	/* memory it has used */
	size_t ownFrames;
	size_t sharedFrames;
	size_t swapped;
	/* cycle-count */
	uLongLong ucycleCount;
	uLongLong kcycleCount;
	/* other stats */
	uint schedCount;
	uint syscalls;
} sExitState;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @return the process-id
 */
pid_t getpid(void);

/**
 * @return the parent-pid of the current process
 */
pid_t getppid(void);

/**
 * Returns the parent-id of the given process
 *
 * @param pid the process-id
 * @return the parent-pid
 */
int getppidof(pid_t pid);

/**
 * Clones the current process
 *
 * @return new pid for parent, 0 for child, < 0 if failed
 */
int fork(void) A_CHECKRET;

/**
 * Exchanges the process-data with the given program
 *
 * @param path the program-path
 * @param args a NULL-terminated array of arguments
 * @return a negative error-code if failed
 */
int exec(const char *path,const char **args);

/**
 * Waits until a child terminates and stores information about it into <state>.
 * Note that a child-process is required and only one thread can wait for a child-process!
 * You may get interrupted by a signal (and may want to call waitChild() again in this case). If so
 * you get ERR_INTERRUPTED as return-value (and errno will be set).
 *
 * @param state the exit-state (may be NULL)
 * @return 0 on success
 */
int waitChild(sExitState *state);

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
int system(const char *cmd);

/**
 * Registers the given function for calling when _exit() is called. Registered functions are
 * called in reverse order. Note that the number of functions is limited!
 *
 * @param func the function
 * @return 0 if successfull
 */
int atexit(fExitFunc func);

/**
 * Calls atexit-functions and then _exit()
 *
 * @param errorCode the error-code for the parent
 */
void exit(int errorCode) A_NORETURN;

#ifdef __cplusplus
}
#endif

#endif /* PROC_H_ */
