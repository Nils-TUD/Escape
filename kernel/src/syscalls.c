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

#include <common.h>
#include <mem/paging.h>
#include <syscalls.h>
#include <syscalls/io.h>
#include <syscalls/mem.h>
#include <syscalls/proc.h>
#include <syscalls/thread.h>
#include <syscalls/other.h>
#include <syscalls/service.h>
#include <syscalls/signals.h>

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
	/* 8 */		{sysc_regService,			2},
	/* 9 */ 	{sysc_unregService,			1},
	/* 10 */	{sysc_changeSize,			1},
	/* 11 */	{sysc_mapPhysical,			2},
	/* 12 */	{sysc_write,				3},
	/* 13 */	{sysc_yield,				0},
	/* 14 */	{sysc_getClient,			3},
	/* 15 */	{sysc_requestIOPorts,		2},
	/* 16 */	{sysc_releaseIOPorts,		2},
	/* 17 */	{sysc_dupFd,				1},
	/* 18 */	{sysc_redirFd,				2},
	/* 19 */	{sysc_wait,					1},
	/* 20 */	{sysc_setSigHandler,		2},
	/* 21 */	{sysc_unsetSigHandler,		1},
	/* 22 */	{sysc_ackSignal,			0},
	/* 23 */	{sysc_sendSignalTo,			3},
	/* 24 */	{sysc_exec,					2},
	/* 25 */	{sysc_eof,					1},
	/* 26 */	{sysc_loadMods,				0},
	/* 27 */	{sysc_sleep,				1},
	/* 28 */	{sysc_seek,					2},
	/* 29 */	{sysc_stat,					2},
	/* 30 */	{sysc_debug,				0},
	/* 31 */	{sysc_createSharedMem,		2},
	/* 32 */	{sysc_joinSharedMem,		1},
	/* 33 */	{sysc_leaveSharedMem,		1},
	/* 34 */	{sysc_destroySharedMem,		1},
	/* 35 */	{sysc_getClientThread,		2},
	/* 36 */	{sysc_lock,					1},
	/* 37 */	{sysc_unlock,				1},
	/* 38 */	{sysc_startThread,			2},
	/* 39 */	{sysc_gettid,				0},
	/* 40 */	{sysc_getThreadCount,		0},
	/* 41 */	{sysc_send,					4},
	/* 42 */	{sysc_receive,				3},
	/* 43 */	{sysc_ioctl,				4},
	/* 44 */	{sysc_setDataReadable,		2},
	/* 45 */	{sysc_getCycles,			0},
	/* 46 */	{sysc_sync,					0},
	/* 47 */	{sysc_link,					2},
	/* 48 */	{sysc_unlink,				1},
	/* 49 */	{sysc_mkdir,				1},
	/* 50 */	{sysc_rmdir,				1},
	/* 51 */	{sysc_mount,				3},
	/* 52 */	{sysc_unmount,				1},
	/* 53 */	{sysc_waitChild,			1},
	/* 54 */	{sysc_tell,					2},
};

void sysc_handle(sIntrptStackFrame *stack) {
	u32 sysCallNo = SYSC_NUMBER(stack);
	if(sysCallNo < ARRAY_SIZE(syscalls)) {
		u32 argCount = syscalls[sysCallNo].argCount;
		u32 ebxSave = stack->ebx;
		/* handle copy-on-write (the first 2 args are passed in registers) */
		/* TODO maybe we need more stack-pages */
		if(argCount > 2)
			paging_isRangeUserWritable((u32)stack->uesp,sizeof(u32) * (argCount - 2));

		/* no error by default */
		SYSC_SETERROR(stack,0);
		syscalls[sysCallNo].handler(stack);

		/* set error-code */
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
