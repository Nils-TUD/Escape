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
#include <assert.h>
#include <errors.h>

/* syscall-handlers */
typedef void (*fSyscall)(sIntrptStackFrame *stack);

/* for syscall-definitions */
typedef struct {
	fSyscall handler;
	uchar argCount;
} sSyscall;

/* our syscalls */
/* TODO */
#ifndef __mmix__
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
	/* 13 */	{sysc_dupFd,				1},
	/* 14 */	{sysc_redirFd,				2},
	/* 15 */	{sysc_wait,					2},
	/* 16 */	{sysc_setSigHandler,		2},
	/* 17 */	{sysc_ackSignal,			0},
	/* 18 */	{sysc_sendSignalTo,			2},
	/* 19 */	{sysc_exec,					2},
	/* 20 */	{sysc_fcntl,				3},
	/* 21 */	{sysc_loadMods,				0},
	/* 22 */	{sysc_sleep,				1},
	/* 23 */	{sysc_seek,					2},
	/* 24 */	{sysc_stat,					2},
	/* 25 */	{sysc_debug,				0},
	/* 26 */	{sysc_createSharedMem,		2},
	/* 27 */	{sysc_joinSharedMem,		1},
	/* 28 */	{sysc_leaveSharedMem,		1},
	/* 29 */	{sysc_destroySharedMem,		1},
	/* 30 */	{sysc_getClient,			2},
	/* 31 */	{sysc_lock,					3},
	/* 32 */	{sysc_unlock,				2},
	/* 33 */	{sysc_startThread,			2},
	/* 34 */	{sysc_gettid,				0},
	/* 35 */	{sysc_getThreadCount,		0},
	/* 36 */	{sysc_send,					4},
	/* 37 */	{sysc_receive,				3},
	/* 38 */	{sysc_getCycles,			0},
	/* 39 */	{sysc_sync,					0},
	/* 40 */	{sysc_link,					2},
	/* 41 */	{sysc_unlink,				1},
	/* 42 */	{sysc_mkdir,				1},
	/* 43 */	{sysc_rmdir,				1},
	/* 44 */	{sysc_mount,				3},
	/* 45 */	{sysc_unmount,				1},
	/* 46 */	{sysc_waitChild,			1},
	/* 47 */	{sysc_tell,					2},
	/* 48 */	{sysc_pipe,					2},
	/* 49 */	{sysc_getConf,				1},
	/* 50 */	{sysc_getWork,				7},
	/* 51 */	{sysc_isterm,				1},
	/* 52 */	{sysc_join,					1},
	/* 53 */	{sysc_suspend,				1},
	/* 54 */	{sysc_resume,				1},
	/* 55 */	{sysc_fstat,				2},
	/* 56 */	{sysc_addRegion,			4},
	/* 57 */	{sysc_setRegProt,			2},
	/* 58 */	{sysc_notify,				2},
	/* 59 */	{sysc_getClientId,			1},
	/* 60 */	{sysc_waitUnlock,			4},
	/* 61 */	{sysc_getenvito,			3},
	/* 62 */	{sysc_getenvto,				3},
	/* 63 */	{sysc_setenv,				2},
#ifdef __i386__
	/* 64 */	{sysc_requestIOPorts,		2},
	/* 65 */	{sysc_releaseIOPorts,		2},
	/* 66 */	{sysc_vm86int,				4},
#endif
};

void sysc_handle(sIntrptStackFrame *stack) {
	uint sysCallNo = SYSC_NUMBER(stack);
	if(sysCallNo >= ARRAY_SIZE(syscalls))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* no error by default */
	SYSC_SETERROR(stack,0);
	syscalls[sysCallNo].handler(stack);
}

uint sysc_getArgCount(uint sysCallNo) {
	if(sysCallNo >= ARRAY_SIZE(syscalls))
		return 0;
	return syscalls[sysCallNo].argCount;
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
