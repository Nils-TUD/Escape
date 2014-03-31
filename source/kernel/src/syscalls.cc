/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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
	/* 0 */
	{getpid,			"getpid",    		0},
	{getppid,			"getppid",    		1},
	{debugc,			"debugc",    		1},
	{fork,				"fork",    			0},
	{exit,				"exit",    			1},
	{open,				"open",    			2},
	{close,				"close",    		1},
	{read,				"read",    			3},
	{createdev,			"createdev",    	3},
	{chgsize,			"chgsize",    		1},

	/* 10 */
	{mmapphys,			"mmapphys",    		2},
	{write,				"write",    		3},
	{yield,				"yield",    		0},
	{dup,				"dup",    			1},
	{redirect,			"redirect",    		2},
	{signal,			"signal",   		2},
	{acksignal,			"acksignal",    	0},
	{kill,				"kill",    			2},
	{exec,				"exec",    			2},
	{fcntl,				"fcntl",    		3},

	/* 20 */
	{loadmods,			"loadmods",    		0},
	{sleep,				"sleep",    		1},
	{seek,				"seek",    			2},
	{stat,				"stat",    			2},
	{startthread,		"startthread",		2},
	{gettid,			"gettid",    		0},
	{getthreadcnt,		"getthreadcnt",     0},
	{send,				"send",    			4},
	{receive,			"receive",    		3},
	{getcycles,			"getcycles",    	0},

	/* 30 */
	{syncfs,			"syncfs",    		1},
	{link,				"link",    			2},
	{unlink,			"unlink",    		1},
	{mkdir,				"mkdir",    		1},
	{rmdir,				"rmdir",    		1},
	{mount,				"mount",    		2},
	{unmount,			"unmount",    		1},
	{waitchild,			"waitchild",    	1},
	{tell,				"tell",    			2},
	{pipe,				"pipe",    			2},

	/* 40 */
	{sysconf,			"sysconf",   		1},
	{getwork,			"getwork",    		6},
	{join,				"join",    			1},
	{fstat,				"fstat",    		2},
	{mmap,				"mmap",    			7},
	{mprotect,			"mprotect",    		2},
	{munmap,			"munmap",    		1},
	{getenvito,			"getenvito",    	3},
	{getenvto,			"getenvto",    		3},
	{setenv,			"setenv",    		2},

	/* 50 */
	{getuid,			"getuid",    		0},
	{setuid,			"setuid",    		1},
	{geteuid,			"geteuid",    		0},
	{seteuid,			"seteuid",    		1},
	{getgid,			"getgid",    		0},
	{setgid,			"setgid",    		1},
	{getegid,			"getegid",    		0},
	{setegid,			"setegid",   		1},
	{chmod,				"chmod",    		2},
	{chown,				"chown",    		3},

	/* 60 */
	{getgroups,			"getgroups",    	2},
	{setgroups,			"setgroups",    	2},
	{isingroup,			"isingroup",    	2},
	{alarm,				"alarm",    		1},
	{tsctotime,			"tsctotime",    	1},
	{semcrt,			"semcrt",			1},
	{semop,				"semop",			1},
	{semdestr,			"semdestr",			1},
	{sendrecv,			"sendrecv",			4},
	{sharefile,			"sharefile",		2},

	/* 70 */
	{sysconfstr,		"sysconfstr",		3},
	{getmsid,			"getmsid",			0},
	{clonems,			"clonems",			0},
	{joinms,			"joinms",			1},
	{mlock,				"mlock",			2},
	{mlockall,			"mlockall",			0},
	{semcrtirq,			"semcrtirq",		1},
#ifdef __i386__
	{reqports,			"reqports",   		2},
	{relports,			"relports",    		2},
	{vm86int,			"vm86int",    		4},
	{vm86start,			"vm86start",    	0},
#else
	{debug,				"debug",    		0},
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
		/* translate "abc://def" to "/dev/abc/def" */
		for(ssize_t i = 0; i < slen; ++i) {
			if(src[i] == ':') {
				if(i + 2 < slen && src[i + 1] == '/' && src[i + 2] == '/') {
					strncpy(dst,"/dev/",SSTRLEN("/dev/"));
					strncpy(dst + SSTRLEN("/dev/"),src,i);
					strnzcpy(dst + SSTRLEN("/dev/") + i,src + i + 2,slen - i - 1);
					return true;
				}
				/* in general, : is invalid in paths */
				else
					return false;
			}
		}

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
