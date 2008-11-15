/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/syscalls.h"
#include "../h/intrpt.h"
#include "../h/proc.h"
#include "../h/video.h"

#define SYSCALL_COUNT 3

/* some convenience-macros */
#define SYSC_ERROR(stack,errorCode) ((stack)->number = (errorCode))
#define SYSC_RET1(stack,val) ((stack)->arg1 = (val))
#define SYSC_RET2(stack,val) ((stack)->arg2 = (val))

/* syscall-handlers */
typedef void (*tSyscallHandler)(tSysCallStack *stack);

/* for syscall-definitions */
typedef struct {
	tSyscallHandler handler;
	u8 argCount;
} tSysCall;

/**
 * Returns the pid of the current process
 */
static void sysc_getpid(tSysCallStack *stack);
/**
 * Returns the parent-pid of the current process
 */
static void sysc_getppid(tSysCallStack *stack);
/**
 * Temporary syscall to print out a character
 */
static void sysc_debugc(tSysCallStack *stack);

/* our syscalls */
static tSysCall syscalls[SYSCALL_COUNT] = {
	/* 0 */	{sysc_getpid,		0},
	/* 1 */	{sysc_getppid,		0},
	/* 2 */ {sysc_debugc,		1}
};

void sysc_handle(tSysCallStack *stack) {
	u32 sysCallNo = stack->number;
	if(sysCallNo < SYSCALL_COUNT) {
		/* no error by default */
		SYSC_ERROR(stack,0);
		syscalls[sysCallNo].handler(stack);
	}
}

static void sysc_getpid(tSysCallStack *stack) {
	tProc *p = proc_getRunning();
	SYSC_RET1(stack,p->pid);
}

static void sysc_getppid(tSysCallStack *stack) {
	tProc *p = proc_getRunning();
	SYSC_RET1(stack,p->parentPid);
}

static void sysc_debugc(tSysCallStack *stack) {
	vid_putchar((s8)stack->arg1);
}
