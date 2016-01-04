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

#include <dbg/console.h>
#include <mem/pagedir.h>
#include <sys/syscalls.h>
#include <task/proc.h>
#include <task/filedesc.h>
#include <ostringstream.h>
#include <assert.h>
#include <common.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <syscalls.h>

/* our syscalls */
const Syscalls::handler_func Syscalls::syscalls[] = {
	/* 0 */
	getpid,
	getppid,
	debugc,
	fork,
	exit,
	open,
	close,
	read,
	createdev,
	chgsize,

	/* 10 */
	mmapphys,
	write,
	yield,
	dup,
	redirect,
	signal,
	acksignal,
	kill,
	exec,
	fcntl,

	/* 20 */
	init,
	sleep,
	seek,
	startthread,
	gettid,
	send,
	receive,
	getcycles,
	syncfs,
	link,

	/* 30 */
	unlink,
	mkdir,
	rmdir,
	mount,
	unmount,
	waitchild,
	tell,
	sysconf,
	getwork,
	join,

	/* 40 */
	fstat,
	mmap,
	mprotect,
	munmap,
	mattr,
	getuid,
	setuid,
	geteuid,
	seteuid,
	getgid,

	/* 50 */
	setgid,
	getegid,
	setegid,
	chmod,
	chown,
	getgroups,
	setgroups,
	isingroup,
	alarm,
	tsctotime,

	/* 60 */
	semcrt,
	semop,
	semdestr,
	sendrecv,
	sharefile,
	cancel,
	creatsibl,
	sysconfstr,
	clonems,
	joinms,

	/* 70 */
	mlock,
	mlockall,
	semcrtirq,
	bindto,
	rename,
	gettimeofday,
	utime,
	truncate,
#if defined(__x86__)
	reqports,
	relports,
#else
	debug,
#endif
};

bool Syscalls::copyPath(char *dst,size_t size,USER const char *src) {
	static_assert(ARRAY_SIZE(syscalls) == SYSCALL_COUNT,"Syscall table not up to date");

	ssize_t slen = UserAccess::strlen(src,MAX_PATH_LEN);
	if(slen < 0 || !PageDir::isInUserSpace((uintptr_t)src,slen))
		return false;
	return UserAccess::strnzcpy(dst,src,size) == 0;
}
