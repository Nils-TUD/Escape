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

#define PRINT_SYSCALLS	0

/* our syscalls */
const Syscalls::Syscall Syscalls::syscalls[] = {
	/* 0 */		{getpid,			"getpid",    		0},
	/* 1 */		{getppid,			"getppid",    		1},
	/* 2 */ 	{debugc,			"debugc",    		1},
	/* 3 */		{fork,				"fork",    			0},
	/* 4 */ 	{exit,				"exit",    			1},
	/* 5 */ 	{open,				"open",    			2},
	/* 6 */ 	{close,				"close",    		1},
	/* 7 */ 	{read,				"read",    			3},
	/* 8 */		{createdev,			"createdev",    	3},
	/* 9 */		{chgsize,			"chgsize",    		1},
	/* 10 */	{mmapphys,			"mmapphys",    		2},
	/* 11 */	{write,				"write",    		3},
	/* 12 */	{yield,				"yield",    		0},
	/* 13 */	{dup,				"dup",    			1},
	/* 14 */	{redirect,			"redirect",    		2},
	/* 15 */	{wait,				"wait",    			3},
	/* 16 */	{signal,			"signal",   		2},
	/* 17 */	{acksignal,			"acksignal",    	0},
	/* 18 */	{kill,				"kill",    			2},
	/* 19 */	{exec,				"exec",    			2},
	/* 20 */	{fcntl,				"fcntl",    		3},
	/* 21 */	{loadmods,			"loadmods",    		0},
	/* 22 */	{sleep,				"sleep",    		1},
	/* 23 */	{seek,				"seek",    			2},
	/* 24 */	{stat,				"stat",    			2},
	/* 25 */	{startthread,		"startthread",		2},
	/* 26 */	{gettid,			"gettid",    		0},
	/* 27 */	{getthreadcnt,		"getthreadcnt",     0},
	/* 28 */	{send,				"send",    			4},
	/* 29 */	{receive,			"receive",    		3},
	/* 30 */	{getcycles,			"getcycles",    	0},
	/* 31 */	{syncfs,			"syncfs",    		1},
	/* 32 */	{link,				"link",    			2},
	/* 33 */	{unlink,			"unlink",    		1},
	/* 34 */	{mkdir,				"mkdir",    		1},
	/* 35 */	{rmdir,				"rmdir",    		1},
	/* 36 */	{mount,				"mount",    		2},
	/* 37 */	{unmount,			"unmount",    		1},
	/* 38 */	{waitchild,			"waitchild",    	1},
	/* 39 */	{tell,				"tell",    			2},
	/* 40 */	{pipe,				"pipe",    			2},
	/* 41 */	{sysconf,			"sysconf",   		1},
	/* 42 */	{getwork,			"getwork",    		6},
	/* 43 */	{join,				"join",    			1},
	/* 44 */	{suspend,			"suspend",    		1},
	/* 45 */	{resume,			"resume",    		1},
	/* 46 */	{fstat,				"fstat",    		2},
	/* 47 */	{mmap,				"mmap",    			7},
	/* 48 */	{mprotect,			"mprotect",    		2},
	/* 49 */	{munmap,			"munmap",    		1},
	/* 50 */	{notify,			"notify",    		2},
	/* 51 */	{getenvito,			"getenvito",    	3},
	/* 52 */	{getenvto,			"getenvto",    		3},
	/* 53 */	{setenv,			"setenv",    		2},
	/* 54 */	{getuid,			"getuid",    		0},
	/* 55 */	{setuid,			"setuid",    		1},
	/* 56 */	{geteuid,			"geteuid",    		0},
	/* 57 */	{seteuid,			"seteuid",    		1},
	/* 58 */	{getgid,			"getgid",    		0},
	/* 59 */	{setgid,			"setgid",    		1},
	/* 60 */	{getegid,			"getegid",    		0},
	/* 61 */	{setegid,			"setegid",   		1},
	/* 62 */	{chmod,				"chmod",    		2},
	/* 63 */	{chown,				"chown",    		3},
	/* 64 */	{getgroups,			"getgroups",    	2},
	/* 65 */	{setgroups,			"setgroups",    	2},
	/* 66 */	{isingroup,			"isingroup",    	2},
	/* 67 */	{alarm,				"alarm",    		1},
	/* 68 */	{tsctotime,			"tsctotime",    	1},
	/* 69 */	{semcrt,			"semcrt",			1},
	/* 70 */	{semop,				"semop",			1},
	/* 71 */	{semdestr,			"semdestr",			1},
	/* 72 */	{sendrecv,			"sendrecv",			4},
	/* 73 */	{sharefile,			"sharefile",		2},
	/* 74 */	{sysconfstr,		"sysconfstr",		3},
	/* 75 */	{getmsid,			"getmsid",			0},
	/* 76 */	{clonems,			"clonems",			0},
	/* 77 */	{joinms,			"joinms",			1},
	/* 78 */	{mlock,				"mlock",			2},
	/* 79 */	{mlockall,			"mlockall",			0},
#ifdef __i386__
	/* 80 */	{reqports,			"reqports",   		2},
	/* 81 */	{relports,			"relports",    		2},
	/* 82 */	{vm86int,			"vm86int",    		4},
	/* 83 */	{vm86start,			"vm86start",    	0},
#else
	/* 80 */	{debug,				"debug",    		0},
#endif
};

void Syscalls::printEntry(A_UNUSED Thread *t,A_UNUSED IntrptStackFrame *stack) {
#if PRINT_SYSCALLS
	uint sysCallNo = SYSC_NUMBER(stack);
	Log::get().writef("[%d:%d:%s] %s(",t->getTid(),t->getProc()->getPid(),t->getProc()->getProgram(),
					  syscalls[sysCallNo].name);
	int i;
	for(i = 1; i <= syscalls[sysCallNo].argCount; ++i) {
		ulong val;
		switch(i) {
			case 1:
				val = SYSC_ARG1(stack);
				break;
			case 2:
				val = SYSC_ARG2(stack);
				break;
			case 3:
				val = SYSC_ARG3(stack);
				break;
			case 4:
				val = SYSC_ARG4(stack);
				break;
			case 5:
				val = SYSC_ARG5(stack);
				break;
			case 6:
				val = SYSC_ARG6(stack);
				break;
			case 7:
				val = SYSC_ARG7(stack);
				break;
		}
		if(i < syscalls[sysCallNo].argCount)
			Log::get().writef("%p,",val);
		else
			Log::get().writef("%p",val);
	}
	Log::get().writef(")...");
#endif
}

void Syscalls::printExit(A_UNUSED IntrptStackFrame *stack) {
#if PRINT_SYSCALLS
	Log::get().writef(" = %p (%d)\n",SYSC_GETRET(stack),SYSC_GETERR(stack));
#endif
}

void Syscalls::handle(Thread *t,IntrptStackFrame *stack) {
	uint sysCallNo = SYSC_NUMBER(stack);
	if(sysCallNo >= ARRAY_SIZE(syscalls)) {
		SYSC_SETERROR(stack,-EINVAL);
		return;
	}

	printEntry(t,stack);
	syscalls[sysCallNo].handler(t,stack);
	printExit(stack);
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
