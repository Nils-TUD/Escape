/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../pub/common.h"
#include "../pub/intrpt.h"
#include "../pub/proc.h"
#include "../pub/video.h"
#include "../pub/vfs.h"
#include "../pub/paging.h"
#include "../pub/string.h"
#include "../pub/util.h"
#include "../pub/debug.h"

#include "../priv/syscalls.h"

/* our syscalls */
static sSyscall syscalls[SYSCALL_COUNT] = {
	/* 0 */	{sysc_getpid,		0},
	/* 1 */	{sysc_getppid,		0},
	/* 2 */ {sysc_debugc,		1},
	/* 3 */	{sysc_fork,			0},
	/* 4 */ {sysc_exit,			1},
	/* 5 */ {sysc_open,			2},
	/* 6 */ {sysc_close,		1},
	/* 7 */ {sysc_read,			3}
};

void sysc_handle(sSysCallStack *stack) {
	u32 sysCallNo = (u32)stack->number;
	if(sysCallNo < SYSCALL_COUNT) {
		/* no error by default */
		SYSC_ERROR(stack,0);
		syscalls[sysCallNo].handler(stack);
	}
}

static void sysc_getpid(sSysCallStack *stack) {
	sProc *p = proc_getRunning();
	SYSC_RET1(stack,p->pid);
}

static void sysc_getppid(sSysCallStack *stack) {
	sProc *p = proc_getRunning();
	SYSC_RET1(stack,p->parentPid);
}

static void sysc_debugc(sSysCallStack *stack) {
	vid_putchar((s8)stack->arg1);
}

static void sysc_fork(sSysCallStack *stack) {
	tPid newPid = proc_getFreePid();
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

static void sysc_exit(sSysCallStack *stack) {
	vid_printf("Process %d exited with exit-code %d\n",proc_getRunning()->pid,stack->arg1);
	proc_suicide();
	proc_switch();
}

static void sysc_open(sSysCallStack *stack) {
	s8 path[255];
	s8 *cleanPath;
	u8 flags;
	tVFSNodeNo nodeNo;
	tFD fd;
	s32 err;

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
	err = vfs_resolvePath(cleanPath,&nodeNo);
	if(err < 0) {
		SYSC_ERROR(stack,err);
		return;
	}

	/* open file */
	err = vfs_openFile(flags,nodeNo,&fd);
	if(err < 0) {
		SYSC_ERROR(stack,err);
		return;
	}

	/* return fd */
	/*dbg_printProcess(proc_getRunning());
	vfs_printNode(vfs_getNode(nodeNo));
	vfs_printGFT();*/
	SYSC_RET1(stack,fd);
}

static void sysc_read(sSysCallStack *stack) {
	tFD fd = (s32)stack->arg1;
	void *buffer = (void*)stack->arg2;
	u32 count = stack->arg3;
	s32 readBytes;

	/* validate count and buffer */
	if(count <= 0) {
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
		return;
	}
	if(!paging_isRangeUserWritable((u32)buffer,count)) {
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
		return;
	}

	/* read */
	readBytes = vfs_readFile(fd,buffer,count);
	if(readBytes < 0) {
		SYSC_ERROR(stack,readBytes);
		return;
	}

	SYSC_RET1(stack,readBytes);
}

static void sysc_close(sSysCallStack *stack) {
	tFD fd = stack->arg1;
	vfs_closeFile(fd);

	/*dbg_printProcess(proc_getRunning());
	vfs_printGFT();*/
}
