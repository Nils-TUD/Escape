/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef PRIVSYSCALLS_H_
#define PRIVSYSCALLS_H_

#include "../pub/common.h"
#include "../pub/syscalls.h"

#define SYSCALL_COUNT 8

/* some convenience-macros */
#define SYSC_ERROR(stack,errorCode) ((stack)->number = (errorCode))
#define SYSC_RET1(stack,val) ((stack)->arg1 = (val))
#define SYSC_RET2(stack,val) ((stack)->arg2 = (val))

/* syscall-handlers */
typedef void (*fSyscall)(sSysCallStack *stack);

/* for syscall-definitions */
typedef struct {
	fSyscall handler;
	u8 argCount;
} sSyscall;

/**
 * Returns the pid of the current process
 */
static void sysc_getpid(sSysCallStack *stack);
/**
 * Returns the parent-pid of the current process
 */
static void sysc_getppid(sSysCallStack *stack);
/**
 * Temporary syscall to print out a character
 */
static void sysc_debugc(sSysCallStack *stack);
/**
 * Clones the current process
 */
static void sysc_fork(sSysCallStack *stack);
/**
 * Destroys the process and issues a context-switch
 */
static void sysc_exit(sSysCallStack *stack);

/**
 * Open-syscall. Opens a given path with given mode and returns the file-descriptor to use
 * to the user.
 */
static void sysc_open(sSysCallStack *stack);

/**
 * Read-syscall. Reads a given number of bytes in a given file at the current position
 */
static void sysc_read(sSysCallStack *stack);

/**
 * Closes the given file-descriptor
 */
static void sysc_close(sSysCallStack *stack);

#endif /* PRIVSYSCALLS_H_ */
