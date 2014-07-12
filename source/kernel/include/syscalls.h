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

#pragma once

#include <common.h>
#include <task/thread.h>
#include <mem/useraccess.h>
#include <interrupts.h>
#include <string.h>

#if defined(__i586__)
#	include <arch/i586/syscalls.h>
#elif defined(__x86_64__)
#	include <arch/x86_64/syscalls.h>
#elif defined(__eco32__)
#	include <arch/eco32/syscalls.h>
#elif defined(__mmix__)
#	include <arch/mmix/syscalls.h>
#endif

class Syscalls {
	typedef int (*handler_func)(Thread *t,IntrptStackFrame *stack);

	/* for syscall-definitions */
	struct Syscall {
		handler_func handler;
		const char *name;
		uchar argCount;
	};

	Syscalls() = delete;

public:
	/**
	 * Handles the syscall for the given stack
	 *
	 * @param t the running thread
	 * @param intrptStack the pointer to the interrupt-stack
	 */
	static void handle(Thread *t,IntrptStackFrame *intrptStack);

	/**
	 * @param str the string
	 * @param len will be set to the length of the string, if len != NULL
	 * @return whether the given string is in user-space
	 */
	static bool isStrInUserSpace(const char *str,size_t *len) {
		ssize_t slen = UserAccess::strlen(str);
		if(slen < 0)
			return false;
		if(len)
			*len = slen;
		return PageDir::isInUserSpace((uintptr_t)str,slen);
	}

private:
	// driver
	static int createdev(Thread *t,IntrptStackFrame *stack);
	static int getwork(Thread *t,IntrptStackFrame *stack);
	static int bindto(Thread *t,IntrptStackFrame *stack);

	// io
	static int open(Thread *t,IntrptStackFrame *stack);
	static int fcntl(Thread *t,IntrptStackFrame *stack);
	static int tell(Thread *t,IntrptStackFrame *stack);
	static int eof(Thread *t,IntrptStackFrame *stack);
	static int seek(Thread *t,IntrptStackFrame *stack);
	static int read(Thread *t,IntrptStackFrame *stack);
	static int write(Thread *t,IntrptStackFrame *stack);
	static int dup(Thread *t,IntrptStackFrame *stack);
	static int redirect(Thread *t,IntrptStackFrame *stack);
	static int close(Thread *t,IntrptStackFrame *stack);
	static int send(Thread *t,IntrptStackFrame *stack);
	static int receive(Thread *t,IntrptStackFrame *stack);
	static int sendrecv(Thread *t,IntrptStackFrame *stack);
	static int cancel(Thread *t,IntrptStackFrame *stack);
	static int sharefile(Thread *t,IntrptStackFrame *stack);
	static int creatsibl(Thread *t,IntrptStackFrame *stack);
	static int stat(Thread *t,IntrptStackFrame *stack);
	static int fstat(Thread *t,IntrptStackFrame *stack);
	static int chmod(Thread *t,IntrptStackFrame *stack);
	static int chown(Thread *t,IntrptStackFrame *stack);
	static int syncfs(Thread *t,IntrptStackFrame *stack);
	static int link(Thread *t,IntrptStackFrame *stack);
	static int unlink(Thread *t,IntrptStackFrame *stack);
	static int rename(Thread *t,IntrptStackFrame *stack);
	static int mkdir(Thread *t,IntrptStackFrame *stack);
	static int rmdir(Thread *t,IntrptStackFrame *stack);
	static int mount(Thread *t,IntrptStackFrame *stack);
	static int unmount(Thread *t,IntrptStackFrame *stack);
	static int getmsid(Thread *t,IntrptStackFrame *stack);
	static int clonems(Thread *t,IntrptStackFrame *stack);
	static int joinms(Thread *t,IntrptStackFrame *stack);

	// mem
	static int chgsize(Thread *t,IntrptStackFrame *stack);
	static int mmap(Thread *t,IntrptStackFrame *stack);
	static int mprotect(Thread *t,IntrptStackFrame *stack);
	static int munmap(Thread *t,IntrptStackFrame *stack);
	static int mmapphys(Thread *t,IntrptStackFrame *stack);
	static int mlock(Thread *t,IntrptStackFrame *stack);
	static int mlockall(Thread *t,IntrptStackFrame *stack);

	// proc
	static int getpid(Thread *t,IntrptStackFrame *stack);
	static int getppid(Thread *t,IntrptStackFrame *stack);
	static int getuid(Thread *t,IntrptStackFrame *stack);
	static int setuid(Thread *t,IntrptStackFrame *stack);
	static int geteuid(Thread *t,IntrptStackFrame *stack);
	static int seteuid(Thread *t,IntrptStackFrame *stack);
	static int getgid(Thread *t,IntrptStackFrame *stack);
	static int setgid(Thread *t,IntrptStackFrame *stack);
	static int getegid(Thread *t,IntrptStackFrame *stack);
	static int setegid(Thread *t,IntrptStackFrame *stack);
	static int getgroups(Thread *t,IntrptStackFrame *stack);
	static int setgroups(Thread *t,IntrptStackFrame *stack);
	static int isingroup(Thread *t,IntrptStackFrame *stack);
	static int fork(Thread *t,IntrptStackFrame *stack);
	static int waitchild(Thread *t,IntrptStackFrame *stack);
	static int exec(Thread *t,IntrptStackFrame *stack);
	static int getenvito(Thread *t,IntrptStackFrame *stack);
	static int getenvto(Thread *t,IntrptStackFrame *stack);
	static int setenv(Thread *t,IntrptStackFrame *stack);

	// signals
	static int signal(Thread *t,IntrptStackFrame *stack);
	static int acksignal(Thread *t,IntrptStackFrame *stack);
	static int kill(Thread *t,IntrptStackFrame *stack);

	// thread
	static int gettid(Thread *t,IntrptStackFrame *stack);
	static int getthreadcnt(Thread *t,IntrptStackFrame *stack);
	static int startthread(Thread *t,IntrptStackFrame *stack);
	static int exit(Thread *t,IntrptStackFrame *stack);
	static int getcycles(Thread *t,IntrptStackFrame *stack);
	static int alarm(Thread *t,IntrptStackFrame *stack);
	static int sleep(Thread *t,IntrptStackFrame *stack);
	static int yield(Thread *t,IntrptStackFrame *stack);
	static int join(Thread *t,IntrptStackFrame *stack);
	static int semcrt(Thread *t,IntrptStackFrame *stack);
	static int semcrtirq(Thread *t,IntrptStackFrame *stack);
	static int semop(Thread *t,IntrptStackFrame *stack);
	static int semdestr(Thread *t,IntrptStackFrame *stack);

	// other
	static int init(Thread *t,IntrptStackFrame *stack);
	static int debugc(Thread *t,IntrptStackFrame *stack);
	static int sysconf(Thread *t,IntrptStackFrame *stack);
	static int sysconfstr(Thread *t,IntrptStackFrame *stack);
	static int gettimeofday(Thread *t,IntrptStackFrame *stack);
	static int tsctotime(Thread *t,IntrptStackFrame *stack);

#if defined(__x86__)
	// x86 specific
	static int reqports(Thread *t,IntrptStackFrame *stack);
	static int relports(Thread *t,IntrptStackFrame *stack);
#else
	static int debug(Thread *t,IntrptStackFrame *stack);
#endif

	/**
	 * @param sysCallNo the syscall-number
	 * @return the number of arguments for the given syscall
	 */
	static uint getArgCount(uint sysCallNo);

	/**
	 * Checks the given path and copies it to <dst>.
	 *
	 * @param dst the destination array
	 * @param size the size of <dst>
	 * @param src the source path
	 * @return true if the string is valid
	 */
	static bool copyPath(char *dst,size_t size,const char *src);

	static void printEntry(Thread *t,IntrptStackFrame *stack);
	static void printExit(Thread *t,IntrptStackFrame *stack);

	/* our syscalls */
	static const Syscall syscalls[];
};
