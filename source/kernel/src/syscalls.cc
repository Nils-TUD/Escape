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
#include <sys/dbg/console.h>
#include <sys/mem/pagedir.h>
#include <sys/task/env.h>
#include <sys/task/proc.h>
#include <sys/syscalls.h>
#include <esc/syscalls.h>
#include <esc/fsinterface.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

/* our syscalls */
const Syscalls::Syscall Syscalls::syscalls[] = {
	/* 0 */		{getpid,			0},
	/* 1 */		{getppid,			1},
	/* 2 */ 	{debugc,			1},
	/* 3 */		{fork,				0},
	/* 4 */ 	{exit,				1},
	/* 5 */ 	{open,				2},
	/* 6 */ 	{close,				1},
	/* 7 */ 	{read,				3},
	/* 8 */		{createdev,			3},
	/* 9 */		{chgsize,			1},
	/* 10 */	{regaddphys,		2},
	/* 11 */	{write,				3},
	/* 12 */	{yield,				0},
	/* 13 */	{dup,				1},
	/* 14 */	{redirect,			2},
	/* 15 */	{wait,				2},
	/* 16 */	{signal,			2},
	/* 17 */	{acksignal,			0},
	/* 18 */	{kill,				2},
	/* 19 */	{exec,				2},
	/* 20 */	{fcntl,				3},
	/* 21 */	{loadmods,			0},
	/* 22 */	{sleep,				1},
	/* 23 */	{seek,				2},
	/* 24 */	{stat,				2},
	/* 25 */	{getclient,			2},
	/* 26 */	{lock,				3},
	/* 27 */	{unlock,			2},
	/* 28 */	{startthread,		2},
	/* 29 */	{gettid,			0},
	/* 30 */	{getthreadcnt,		0},
	/* 31 */	{send,				4},
	/* 32 */	{receive,			3},
	/* 33 */	{getcycles,			0},
	/* 34 */	{sync,				0},
	/* 35 */	{link,				2},
	/* 36 */	{unlink,			1},
	/* 37 */	{mkdir,				1},
	/* 38 */	{rmdir,				1},
	/* 39 */	{mount,				3},
	/* 40 */	{unmount,			1},
	/* 41 */	{waitchild,			1},
	/* 42 */	{tell,				2},
	/* 43 */	{pipe,				2},
	/* 44 */	{sysconf,			1},
	/* 45 */	{getwork,			7},
	/* 46 */	{join,				1},
	/* 47 */	{suspend,			1},
	/* 48 */	{resume,			1},
	/* 49 */	{fstat,				2},
	/* 50 */	{mmap,				7},
	/* 51 */	{mprotect,			2},
	/* 52 */	{munmap,			1},
	/* 53 */	{notify,			2},
	/* 54 */	{getclientid,		1},
	/* 55 */	{waitunlock,		4},
	/* 56 */	{getenvito,			3},
	/* 57 */	{getenvto,			3},
	/* 58 */	{setenv,			2},
	/* 59 */	{getuid,			0},
	/* 60 */	{setuid,			1},
	/* 61 */	{geteuid,			0},
	/* 62 */	{seteuid,			1},
	/* 63 */	{getgid,			0},
	/* 64 */	{setgid,			1},
	/* 65 */	{getegid,			0},
	/* 66 */	{setegid,			1},
	/* 67 */	{chmod,				2},
	/* 68 */	{chown,				3},
	/* 69 */	{getgroups,			2},
	/* 70 */	{setgroups,			2},
	/* 71 */	{isingroup,			2},
	/* 72 */	{alarm,				1},
	/* 73 */	{tsctotime,			1},
#ifdef __i386__
	/* 74 */	{reqports,			2},
	/* 75 */	{relports,			2},
	/* 76 */	{vm86int,			4},
	/* 77 */	{vm86start,			0},
#else
	/* 74 */	{debug,				0},
#endif
};

void Syscalls::handle(Thread *t,IntrptStackFrame *stack) {
	uint sysCallNo = SYSC_NUMBER(stack);
	if(sysCallNo >= ARRAY_SIZE(syscalls)) {
		SYSC_SETERROR(stack,-EINVAL);
		return;
	}

	syscalls[sysCallNo].handler(t,stack);
}

uint Syscalls::getArgCount(uint sysCallNo) {
	if(sysCallNo >= ARRAY_SIZE(syscalls))
		return 0;
	return syscalls[sysCallNo].argCount;
}

bool Syscalls::absolutizePath(char *dst,size_t size,const char *src) {
	size_t len = 0;
	ssize_t slen = strnlen(src,MAX_PATH_LEN);
	if(slen < 0 || !PageDir::isInUserSpace((uintptr_t)src,slen))
		return false;
	if(*src != '/') {
		pid_t pid = Proc::getRunning();
		if(Env::get(pid,"CWD",dst,size)) {
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
