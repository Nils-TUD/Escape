/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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
#include <sys/task/env.h>
#include <sys/syscalls.h>
#include <sys/syscalls/io.h>
#include <sys/syscalls/mem.h>
#include <sys/syscalls/proc.h>
#include <sys/syscalls/thread.h>
#include <sys/syscalls/other.h>
#include <sys/syscalls/driver.h>
#include <sys/syscalls/signals.h>
#include <esc/syscalls.h>
#include <esc/fsinterface.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

/* syscall-handlers */
typedef int (*fSyscall)(sThread *t,sIntrptStackFrame *stack);

/* for syscall-definitions */
typedef struct {
	fSyscall handler;
	uchar argCount;
} sSyscall;

/* our syscalls */
static const sSyscall syscalls[] = {
	/* 0 */		{sysc_getpid,				0},
	/* 1 */		{sysc_getppid,				1},
	/* 2 */ 	{sysc_debugc,				1},
	/* 3 */		{sysc_fork,					0},
	/* 4 */ 	{sysc_exit,					1},
	/* 5 */ 	{sysc_open,					2},
	/* 6 */ 	{sysc_close,				1},
	/* 7 */ 	{sysc_read,					3},
	/* 8 */		{sysc_createdev,			3},
	/* 9 */		{sysc_chgsize,				1},
	/* 10 */	{sysc_regaddphys,				2},
	/* 11 */	{sysc_write,				3},
	/* 12 */	{sysc_yield,				0},
	/* 13 */	{sysc_dup,					1},
	/* 14 */	{sysc_redirect,				2},
	/* 15 */	{sysc_wait,					2},
	/* 16 */	{sysc_signal,				2},
	/* 17 */	{sysc_acksignal,			0},
	/* 18 */	{sysc_kill,					2},
	/* 19 */	{sysc_exec,					2},
	/* 20 */	{sysc_fcntl,				3},
	/* 21 */	{sysc_loadmods,				0},
	/* 22 */	{sysc_sleep,				1},
	/* 23 */	{sysc_seek,					2},
	/* 24 */	{sysc_stat,					2},
	/* 25 */	{sysc_debug,				0},
	/* 26 */	{sysc_shmcrt,				2},
	/* 27 */	{sysc_shmjoin,				1},
	/* 28 */	{sysc_shmleave,				1},
	/* 29 */	{sysc_shmdel,				1},
	/* 30 */	{sysc_getclient,			2},
	/* 31 */	{sysc_lock,					3},
	/* 32 */	{sysc_unlock,				2},
	/* 33 */	{sysc_startthread,			2},
	/* 34 */	{sysc_gettid,				0},
	/* 35 */	{sysc_getthreadcnt,			0},
	/* 36 */	{sysc_send,					4},
	/* 37 */	{sysc_receive,				3},
	/* 38 */	{sysc_getcycles,			0},
	/* 39 */	{sysc_sync,					0},
	/* 40 */	{sysc_link,					2},
	/* 41 */	{sysc_unlink,				1},
	/* 42 */	{sysc_mkdir,				1},
	/* 43 */	{sysc_rmdir,				1},
	/* 44 */	{sysc_mount,				3},
	/* 45 */	{sysc_unmount,				1},
	/* 46 */	{sysc_waitchild,			1},
	/* 47 */	{sysc_tell,					2},
	/* 48 */	{sysc_pipe,					2},
	/* 49 */	{sysc_sysconf,				1},
	/* 50 */	{sysc_getwork,				7},
	/* 51 */	{sysc_join,					1},
	/* 52 */	{sysc_suspend,				1},
	/* 53 */	{sysc_resume,				1},
	/* 54 */	{sysc_fstat,				2},
	/* 55 */	{sysc_regadd,				4},
	/* 56 */	{sysc_regctrl,				2},
	/* 57 */	{sysc_regrem,				1},
	/* 58 */	{sysc_notify,				2},
	/* 59 */	{sysc_getclientid,			1},
	/* 60 */	{sysc_waitunlock,			4},
	/* 61 */	{sysc_getenvito,			3},
	/* 62 */	{sysc_getenvto,				3},
	/* 63 */	{sysc_setenv,				2},
	/* 64 */	{sysc_getuid,				0},
	/* 65 */	{sysc_setuid,				1},
	/* 66 */	{sysc_geteuid,				0},
	/* 67 */	{sysc_seteuid,				1},
	/* 68 */	{sysc_getgid,				0},
	/* 69 */	{sysc_setgid,				1},
	/* 70 */	{sysc_getegid,				0},
	/* 71 */	{sysc_setegid,				1},
	/* 72 */	{sysc_chmod,				2},
	/* 73 */	{sysc_chown,				3},
	/* 74 */	{sysc_getgroups,			2},
	/* 75 */	{sysc_setgroups,			2},
	/* 76 */	{sysc_isingroup,			2},
	/* 77 */	{sysc_alarm,				1},
	/* 78 */	{sysc_regaddmod,				2},
	/* 79 */	{sysc_tsctotime,			1},
#ifdef __i386__
	/* 80 */	{sysc_reqports,				2},
	/* 81 */	{sysc_relports,				2},
	/* 82 */	{sysc_vm86int,				4},
	/* 83 */	{sysc_vm86start,			0},
#endif
};

void sysc_handle(sThread *t,sIntrptStackFrame *stack) {
	uint sysCallNo = SYSC_NUMBER(stack);
	if(sysCallNo >= ARRAY_SIZE(syscalls)) {
		SYSC_SETERROR(stack,-EINVAL);
		return;
	}

	syscalls[sysCallNo].handler(t,stack);
}

uint sysc_getArgCount(uint sysCallNo) {
	if(sysCallNo >= ARRAY_SIZE(syscalls))
		return 0;
	return syscalls[sysCallNo].argCount;
}

bool sysc_isStrInUserSpace(const char *str,size_t *len) {
	size_t slen = strlen(str);
	if(len)
		*len = slen;
	return paging_isInUserSpace((uintptr_t)str,slen);
}

bool sysc_absolutize_path(char *dst,size_t size,const char *src) {
	size_t len = 0;
	ssize_t slen = strnlen(src,MAX_PATH_LEN);
	if(slen < 0 || !paging_isInUserSpace((uintptr_t)src,slen))
		return false;
	if(*src != '/') {
		pid_t pid = proc_getRunning();
		if(env_get(pid,"CWD",dst,size)) {
			len = strlen(dst);
			if(len < size - 1 && dst[len - 1] != '/') {
				dst[len++] = '/';
				dst[len] = '\0';
			}
		}
		else {
			/* assume '/' */
			len = 1;
			dst[0] = '/';
			dst[1] = '\0';
		}
	}
	strnzcpy(dst + len,src,size - len);
	return true;
}
