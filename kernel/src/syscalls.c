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

#include <sys/common.h>
#include <sys/mem/paging.h>
#include <sys/syscalls.h>
#include <sys/syscalls/io.h>
#include <sys/syscalls/mem.h>
#include <sys/syscalls/proc.h>
#include <sys/syscalls/thread.h>
#include <sys/syscalls/other.h>
#include <sys/syscalls/driver.h>
#include <sys/syscalls/signals.h>
#include <errors.h>

/* syscall-handlers */
typedef void (*fSyscall)(sIntrptStackFrame *stack);

/* for syscall-definitions */
typedef struct {
	fSyscall handler;
	uchar argCount;
} sSyscall;

/* TODO */
#ifdef __i386__
/* our syscalls */
static sSyscall syscalls[] = {
	/* 0 */		{sysc_getpid,				0},
	/* 1 */		{sysc_getppid,				1},
	/* 2 */ 	{sysc_debugc,				1},
	/* 3 */		{sysc_fork,					0},
	/* 4 */ 	{sysc_exit,					1},
	/* 5 */ 	{sysc_open,					2},
	/* 6 */ 	{sysc_close,				1},
	/* 7 */ 	{sysc_read,					3},
	/* 8 */		{sysc_regDriver,			2},
	/* 9 */		{sysc_changeSize,			1},
	/* 10 */	{sysc_mapPhysical,			2},
	/* 11 */	{sysc_write,				3},
	/* 12 */	{sysc_yield,				0},
	/* 13 */	{sysc_requestIOPorts,		2},
	/* 14 */	{sysc_releaseIOPorts,		2},
	/* 15 */	{sysc_dupFd,				1},
	/* 16 */	{sysc_redirFd,				2},
	/* 17 */	{sysc_wait,					2},
	/* 18 */	{sysc_setSigHandler,		2},
	/* 19 */	{sysc_ackSignal,			0},
	/* 20 */	{sysc_sendSignalTo,			2},
	/* 21 */	{sysc_exec,					2},
	/* 22 */	{sysc_fcntl,				3},
	/* 23 */	{sysc_loadMods,				0},
	/* 24 */	{sysc_sleep,				1},
	/* 25 */	{sysc_seek,					2},
	/* 26 */	{sysc_stat,					2},
	/* 27 */	{sysc_debug,				0},
	/* 28 */	{sysc_createSharedMem,		2},
	/* 29 */	{sysc_joinSharedMem,		1},
	/* 30 */	{sysc_leaveSharedMem,		1},
	/* 31 */	{sysc_destroySharedMem,		1},
	/* 32 */	{sysc_getClient,			2},
	/* 33 */	{sysc_lock,					3},
	/* 34 */	{sysc_unlock,				2},
	/* 35 */	{sysc_startThread,			2},
	/* 36 */	{sysc_gettid,				0},
	/* 37 */	{sysc_getThreadCount,		0},
	/* 38 */	{sysc_send,					4},
	/* 39 */	{sysc_receive,				3},
	/* 40 */	{sysc_getCycles,			0},
	/* 41 */	{sysc_sync,					0},
	/* 42 */	{sysc_link,					2},
	/* 43 */	{sysc_unlink,				1},
	/* 44 */	{sysc_mkdir,				1},
	/* 45 */	{sysc_rmdir,				1},
	/* 46 */	{sysc_mount,				3},
	/* 47 */	{sysc_unmount,				1},
	/* 48 */	{sysc_waitChild,			1},
	/* 49 */	{sysc_tell,					2},
	/* 50 */	{sysc_pipe,					2},
	/* 51 */	{sysc_getConf,				1},
	/* 52 */	{sysc_vm86int,				4},
	/* 53 */	{sysc_getWork,				7},
	/* 54 */	{sysc_isterm,				1},
	/* 55 */	{sysc_join,					1},
	/* 56 */	{sysc_suspend,				1},
	/* 57 */	{sysc_resume,				1},
	/* 58 */	{sysc_fstat,				2},
	/* 59 */	{sysc_addRegion,			4},
	/* 60 */	{sysc_setRegProt,			2},
	/* 61 */	{sysc_notify,				2},
	/* 62 */	{sysc_getClientId,			1},
	/* 63 */	{sysc_waitUnlock,			4},
	/* 64 */	{sysc_getenvito,			3},
	/* 65 */	{sysc_getenvto,				3},
	/* 66 */	{sysc_setenv,				2},
};

void sysc_handle(sIntrptStackFrame *stack) {
	uint argCount,ebxSave;
	uint sysCallNo = SYSC_NUMBER(stack);
	if(sysCallNo >= ARRAY_SIZE(syscalls))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	argCount = syscalls[sysCallNo].argCount;
	ebxSave = stack->ebx;
	/* handle copy-on-write (the first 2 args are passed in registers) */
	if(argCount > 2) {
		/* if the arguments are not mapped, return an error */
		if(!paging_isRangeUserWritable((uintptr_t)stack->uesp,sizeof(uint) * (argCount - 2)))
			SYSC_ERROR(stack,ERR_INVALID_ARGS);
	}

	/* no error by default */
	SYSC_SETERROR(stack,0);
	syscalls[sysCallNo].handler(stack);

	/* set error-code (not for ackSignal) */
	if(sysCallNo != 19) {
		stack->ecx = stack->ebx;
		stack->ebx = ebxSave;
	}
}
#endif

bool sysc_isStringReadable(const char *str) {
	uintptr_t addr;
	size_t rem;
	/* null is a special case */
	if(str == NULL)
		return false;

	/* check if it is readable */
	addr = (uintptr_t)str & ~(PAGE_SIZE - 1);
	rem = (addr + PAGE_SIZE) - (uintptr_t)str;
	while(paging_isRangeUserReadable(addr,PAGE_SIZE)) {
		while(rem-- > 0 && *str)
			str++;
		if(!*str)
			return true;
		addr += PAGE_SIZE;
		rem = PAGE_SIZE;
	}
	return false;
}
