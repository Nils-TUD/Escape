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
#include "../h/vfs.h"
#include "../h/paging.h"
#include "../h/string.h"
#include "../h/util.h"
#include "../h/debug.h"

#define SYSCALL_COUNT 7

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
/**
 * Clones the current process
 */
static void sysc_fork(tSysCallStack *stack);
/**
 * Destroys the process and issues a context-switch
 */
static void sysc_exit(tSysCallStack *stack);

/**
 * Open-syscall. Opens a given path with given mode and returns the file-descriptor to use
 * to the user.
 */
static void sysc_open(tSysCallStack *stack);

/**
 * Closes the given file-descriptor
 */
static void sysc_close(tSysCallStack *stack);

/* our syscalls */
static tSysCall syscalls[SYSCALL_COUNT] = {
	/* 0 */	{sysc_getpid,		0},
	/* 1 */	{sysc_getppid,		0},
	/* 2 */ {sysc_debugc,		1},
	/* 3 */	{sysc_fork,			0},
	/* 4 */ {sysc_exit,			1},
	/* 5 */ {sysc_open,			2},
	/* 6 */ {sysc_close,		1}
};

void sysc_handle(tSysCallStack *stack) {
	u32 sysCallNo = (u32)stack->number;
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

static void sysc_fork(tSysCallStack *stack) {
	u16 newPid = proc_getFreePid();
	s32 res = proc_clone(newPid);
	/* error? */
	if(res == -1) {
		SYSC_RET1(stack,-1);
	}
	/* child? */
	else if(res == 1) {
		SYSC_RET1(stack,0);
	}
	/* parent */
	else {
		SYSC_RET1(stack,newPid);
	}
}

static void sysc_exit(tSysCallStack *stack) {
	vid_printf("Process %d exited with exit-code %d\n",proc_getRunning()->pid,stack->arg1);
	proc_suicide();
	proc_switch();
}

static void sysc_open(tSysCallStack *stack) {
	s8 path[255];
	s8 *cleanPath;
	bool isReal;
	u8 flags;
	s32 nodeNo,fd,err;

	/* copy path */
	copyUserToKernel((u8*)stack->arg1,(u8*)path,MIN(strlen((string)stack->arg1) + 1,255));

	/* check flags */
	flags = ((u8)stack->arg2) & (GFT_WRITE | GFT_READ);
	if((flags & (GFT_READ | GFT_WRITE)) == 0) {
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
		return;
	}

	/* resolve path */
	cleanPath = vfs_cleanPath(path);
	nodeNo = vfs_resolvePath(cleanPath,&isReal);
	if(nodeNo < 0) {
		SYSC_ERROR(stack,nodeNo);
		return;
	}
	flags |= isReal ? GFT_REAL : GFT_VIRTUAL;

	/* open file */
	fd = vfs_openFile(flags,nodeNo);
	if(fd < 0) {
		SYSC_ERROR(stack,fd);
		return;
	}

	err = proc_openFile(fd);
	if(err < 0) {
		vfs_closeFile(fd);
		SYSC_ERROR(stack,err);
		return;
	}

	/* return fd */
	dbg_printProcess(proc_getRunning());
	vfs_printNode(vfs_getNode(nodeNo));
	vfs_printGFT();
	SYSC_RET1(stack,fd);
}

static void sysc_close(tSysCallStack *stack) {
	u32 fd = stack->arg1;
	proc_closeFile(fd);
	vfs_closeFile(fd);

	dbg_printProcess(proc_getRunning());
	vfs_printGFT();
}
