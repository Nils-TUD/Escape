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
	u8 argCount;
} sSyscall;

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
	/* 9 */ 	{sysc_unregDriver,			1},
	/* 10 */	{sysc_changeSize,			1},
	/* 11 */	{sysc_mapPhysical,			2},
	/* 12 */	{sysc_write,				3},
	/* 13 */	{sysc_yield,				0},
	/* 14 */	{sysc_requestIOPorts,		2},
	/* 15 */	{sysc_releaseIOPorts,		2},
	/* 16 */	{sysc_dupFd,				1},
	/* 17 */	{sysc_redirFd,				2},
	/* 18 */	{sysc_wait,					1},
	/* 19 */	{sysc_setSigHandler,		2},
	/* 20 */	{sysc_ackSignal,			0},
	/* 21 */	{sysc_sendSignalTo,			3},
	/* 22 */	{sysc_exec,					2},
	/* 23 */	{sysc_eof,					1},
	/* 24 */	{sysc_loadMods,				0},
	/* 25 */	{sysc_sleep,				1},
	/* 26 */	{sysc_seek,					2},
	/* 27 */	{sysc_stat,					2},
	/* 28 */	{sysc_debug,				0},
	/* 29 */	{sysc_createSharedMem,		2},
	/* 30 */	{sysc_joinSharedMem,		1},
	/* 31 */	{sysc_leaveSharedMem,		1},
	/* 32 */	{sysc_destroySharedMem,		1},
	/* 33 */	{sysc_getClient,			2},
	/* 34 */	{sysc_lock,					3},
	/* 35 */	{sysc_unlock,				2},
	/* 36 */	{sysc_startThread,			2},
	/* 37 */	{sysc_gettid,				0},
	/* 38 */	{sysc_getThreadCount,		0},
	/* 49 */	{sysc_send,					4},
	/* 40 */	{sysc_receive,				3},
	/* 41 */	{sysc_hasMsg,				1},
	/* 42 */	{sysc_setDataReadable,		2},
	/* 43 */	{sysc_getCycles,			0},
	/* 44 */	{sysc_sync,					0},
	/* 45 */	{sysc_link,					2},
	/* 46 */	{sysc_unlink,				1},
	/* 47 */	{sysc_mkdir,				1},
	/* 48 */	{sysc_rmdir,				1},
	/* 49 */	{sysc_mount,				3},
	/* 50 */	{sysc_unmount,				1},
	/* 51 */	{sysc_waitChild,			1},
	/* 52 */	{sysc_tell,					2},
	/* 53 */	{sysc_pipe,					2},
	/* 54 */	{sysc_getConf,				1},
	/* 55 */	{sysc_vm86int,				4},
	/* 56 */	{sysc_getWork,				7},
	/* 57 */	{sysc_isterm,				1},
	/* 58 */	{sysc_join,					1},
	/* 59 */	{sysc_suspend,				1},
	/* 60 */	{sysc_resume,				1},
	/* 61 */	{sysc_fstat,				2},
	/* 62 */	{sysc_addRegion,			4},
	/* 63 */	{sysc_setRegProt,			2},
	/* 64 */	{sysc_notify,				2},
	/* 65 */	{sysc_getClientId,			1},
	/* 66 */	{sysc_waitUnlock,			3},
};

void sysc_handle(sIntrptStackFrame *stack) {
	u32 argCount,ebxSave;
	u32 sysCallNo = SYSC_NUMBER(stack);
	if(sysCallNo >= ARRAY_SIZE(syscalls))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	argCount = syscalls[sysCallNo].argCount;
	ebxSave = stack->ebx;
	/* handle copy-on-write (the first 2 args are passed in registers) */
	if(argCount > 2) {
		/* if the arguments are not mapped, return an error */
		if(!paging_isRangeUserWritable((u32)stack->uesp,sizeof(u32) * (argCount - 2)))
			SYSC_ERROR(stack,ERR_INVALID_ARGS);
	}

	/* no error by default */
	SYSC_SETERROR(stack,0);
	syscalls[sysCallNo].handler(stack);

	/* set error-code (not for ackSignal) */
	if(sysCallNo != 20) {
		stack->ecx = stack->ebx;
		stack->ebx = ebxSave;
	}
}

bool sysc_isStringReadable(const char *str) {
	u32 addr,rem;
	/* null is a special case */
	if(str == NULL)
		return false;

	/* check if it is readable */
	addr = (u32)str & ~(PAGE_SIZE - 1);
	rem = (addr + PAGE_SIZE) - (u32)str;
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
