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

#pragma once

#include <sys/common.h>
#include <sys/task/thread.h>
#include <sys/intrpt.h>
#include <string.h>

#ifdef __i386__
#include <sys/arch/i586/syscalls.h>
#endif
#ifdef __eco32__
#include <sys/arch/eco32/syscalls.h>
#endif
#ifdef __mmix__
#include <sys/arch/mmix/syscalls.h>
#endif

class Syscalls {
	typedef int (*handler_func)(Thread *t,sIntrptStackFrame *stack);

	/* for syscall-definitions */
	struct Syscall {
		handler_func handler;
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
	static void handle(Thread *t,sIntrptStackFrame *intrptStack);

	/**
	 * @param str the string
	 * @param len will be set to the length of the string, if len != NULL
	 * @return whether the given string is in user-space
	 */
	static bool isStrInUserSpace(const char *str,size_t *len) {
		size_t slen = strlen(str);
		if(len)
			*len = slen;
		return PageDir::isInUserSpace((uintptr_t)str,slen);
	}

private:
	// driver
	static int createdev(Thread *t,sIntrptStackFrame *stack);
	static int getclientid(Thread *t,sIntrptStackFrame *stack);
	static int getclient(Thread *t,sIntrptStackFrame *stack);
	static int getwork(Thread *t,sIntrptStackFrame *stack);

	// io
	static int open(Thread *t,sIntrptStackFrame *stack);
	static int fcntl(Thread *t,sIntrptStackFrame *stack);
	static int pipe(Thread *t,sIntrptStackFrame *stack);
	static int tell(Thread *t,sIntrptStackFrame *stack);
	static int eof(Thread *t,sIntrptStackFrame *stack);
	static int seek(Thread *t,sIntrptStackFrame *stack);
	static int read(Thread *t,sIntrptStackFrame *stack);
	static int write(Thread *t,sIntrptStackFrame *stack);
	static int dup(Thread *t,sIntrptStackFrame *stack);
	static int redirect(Thread *t,sIntrptStackFrame *stack);
	static int close(Thread *t,sIntrptStackFrame *stack);
	static int send(Thread *t,sIntrptStackFrame *stack);
	static int receive(Thread *t,sIntrptStackFrame *stack);
	static int stat(Thread *t,sIntrptStackFrame *stack);
	static int fstat(Thread *t,sIntrptStackFrame *stack);
	static int chmod(Thread *t,sIntrptStackFrame *stack);
	static int chown(Thread *t,sIntrptStackFrame *stack);
	static int sync(Thread *t,sIntrptStackFrame *stack);
	static int link(Thread *t,sIntrptStackFrame *stack);
	static int unlink(Thread *t,sIntrptStackFrame *stack);
	static int mkdir(Thread *t,sIntrptStackFrame *stack);
	static int rmdir(Thread *t,sIntrptStackFrame *stack);
	static int mount(Thread *t,sIntrptStackFrame *stack);
	static int unmount(Thread *t,sIntrptStackFrame *stack);

	// mem
	static int chgsize(Thread *t,sIntrptStackFrame *stack);
	static int mmap(Thread *t,sIntrptStackFrame *stack);
	static int mprotect(Thread *t,sIntrptStackFrame *stack);
	static int munmap(Thread *t,sIntrptStackFrame *stack);
	static int regaddphys(Thread *t,sIntrptStackFrame *stack);

	// proc
	static int getpid(Thread *t,sIntrptStackFrame *stack);
	static int getppid(Thread *t,sIntrptStackFrame *stack);
	static int getuid(Thread *t,sIntrptStackFrame *stack);
	static int setuid(Thread *t,sIntrptStackFrame *stack);
	static int geteuid(Thread *t,sIntrptStackFrame *stack);
	static int seteuid(Thread *t,sIntrptStackFrame *stack);
	static int getgid(Thread *t,sIntrptStackFrame *stack);
	static int setgid(Thread *t,sIntrptStackFrame *stack);
	static int getegid(Thread *t,sIntrptStackFrame *stack);
	static int setegid(Thread *t,sIntrptStackFrame *stack);
	static int getgroups(Thread *t,sIntrptStackFrame *stack);
	static int setgroups(Thread *t,sIntrptStackFrame *stack);
	static int isingroup(Thread *t,sIntrptStackFrame *stack);
	static int fork(Thread *t,sIntrptStackFrame *stack);
	static int waitchild(Thread *t,sIntrptStackFrame *stack);
	static int exec(Thread *t,sIntrptStackFrame *stack);
	static int getenvito(Thread *t,sIntrptStackFrame *stack);
	static int getenvto(Thread *t,sIntrptStackFrame *stack);
	static int setenv(Thread *t,sIntrptStackFrame *stack);

	// signals
	static int signal(Thread *t,sIntrptStackFrame *stack);
	static int acksignal(Thread *t,sIntrptStackFrame *stack);
	static int kill(Thread *t,sIntrptStackFrame *stack);

	// thread
	static int gettid(Thread *t,sIntrptStackFrame *stack);
	static int getthreadcnt(Thread *t,sIntrptStackFrame *stack);
	static int startthread(Thread *t,sIntrptStackFrame *stack);
	static int exit(Thread *t,sIntrptStackFrame *stack);
	static int getcycles(Thread *t,sIntrptStackFrame *stack);
	static int alarm(Thread *t,sIntrptStackFrame *stack);
	static int sleep(Thread *t,sIntrptStackFrame *stack);
	static int yield(Thread *t,sIntrptStackFrame *stack);
	static int wait(Thread *t,sIntrptStackFrame *stack);
	static int waitunlock(Thread *t,sIntrptStackFrame *stack);
	static int notify(Thread *t,sIntrptStackFrame *stack);
	static int lock(Thread *t,sIntrptStackFrame *stack);
	static int unlock(Thread *t,sIntrptStackFrame *stack);
	static int join(Thread *t,sIntrptStackFrame *stack);
	static int suspend(Thread *t,sIntrptStackFrame *stack);
	static int resume(Thread *t,sIntrptStackFrame *stack);

	// other
	static int loadmods(Thread *t,sIntrptStackFrame *stack);
	static int debugc(Thread *t,sIntrptStackFrame *stack);
	static int debug(Thread *t,sIntrptStackFrame *stack);
	static int sysconf(Thread *t,sIntrptStackFrame *stack);
	static int tsctotime(Thread *t,sIntrptStackFrame *stack);

#ifdef __i386__
	// x86 specific
	static int reqports(Thread *t,sIntrptStackFrame *stack);
	static int relports(Thread *t,sIntrptStackFrame *stack);
	static int vm86start(Thread *t,sIntrptStackFrame *stack);
	static int vm86int(Thread *t,sIntrptStackFrame *stack);
#endif

	/**
	 * @param sysCallNo the syscall-number
	 * @return the number of arguments for the given syscall
	 */
	static uint getArgCount(uint sysCallNo);

	/**
	 * Checks the given path and makes it absolute.
	 *
	 * @param dst the destination array
	 * @param size the size of <dst>
	 * @param src the source path to absolutize
	 * @return true if the string is valid
	 */
	static bool absolutizePath(char *dst,size_t size,const char *src);

	/* our syscalls */
	static const Syscall syscalls[];
};
