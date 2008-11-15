/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/syscalls.h"
#include "../h/intrpt.h"
#include "../h/proc.h"

#define SYSCALL_COUNT 1

/* some convenience-macros */
#define SYSC_ERROR(stack,errorCode) ((stack)->sysCallNo = (errorCode))
#define SYSC_RET1(stack,val) ((stack)->sysCallArg1 = (val))
#define SYSC_RET2(stack,val) ((stack)->sysCallArg2 = (val))

/* syscall-handlers */
typedef void (*tSyscallHandler)(tIntrptStackFrame *stack);

/* for syscall-definitions */
typedef struct {
	tSyscallHandler handler;
	u8 argCount;
} tSysCall;

/**
 * Returns the pid of the current process
 */
static void sysc_getpid(tIntrptStackFrame *stack);
/**
 * Returns the parent-pid of the current process
 */
static void sysc_getppid(tIntrptStackFrame *stack);

/* our syscalls */
static tSysCall syscalls[SYSCALL_COUNT] = {
	/* 0 */	{sysc_getpid,0},
	/* 1 */	{sysc_getppid,0}
};

void sysc_handle(tIntrptStackFrame *stack) {
	if(stack->sysCallNo < SYSCALL_COUNT) {
		/* no error by default */
		SYSC_ERROR(stack,0);
		syscalls[stack->sysCallNo].handler(stack);
	}
}

static void sysc_getpid(tIntrptStackFrame *stack) {
	tProc *p = proc_getRunning();
	SYSC_RET1(stack,p->pid);
}

static void sysc_getppid(tIntrptStackFrame *stack) {
	tProc *p = proc_getRunning();
	SYSC_RET1(stack,p->parentPid);
}
