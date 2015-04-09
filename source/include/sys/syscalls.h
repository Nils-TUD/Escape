/**
 * $Id$
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

#ifndef IN_ASM
/* the syscall-numbers */
enum {
	/* 0 */
	SYSCALL_PID,
	SYSCALL_PPID,
	SYSCALL_DEBUGCHAR,
	SYSCALL_FORK,
	SYSCALL_EXIT,
	SYSCALL_OPEN,
	SYSCALL_CLOSE,
	SYSCALL_READ,
	SYSCALL_CRTDEV,
	SYSCALL_CHGSIZE,

	/* 10 */
	SYSCALL_MAPPHYS,
	SYSCALL_WRITE,
	SYSCALL_YIELD,
	SYSCALL_DUPFD,
	SYSCALL_REDIRFD,
	SYSCALL_SETSIGH,
	SYSCALL_ACKSIG,
	SYSCALL_SENDSIG,
	SYSCALL_EXEC,
	SYSCALL_FCNTL,

	/* 20 */
	SYSCALL_INIT,
	SYSCALL_SLEEP,
	SYSCALL_SEEK,
	SYSCALL_STAT,
	SYSCALL_STARTTHREAD,
	SYSCALL_GETTID,
	SYSCALL_SEND,
	SYSCALL_RECEIVE,
	SYSCALL_GETCYCLES,
	SYSCALL_SYNCFS,

	/* 30 */
	SYSCALL_LINK,
	SYSCALL_UNLINK,
	SYSCALL_MKDIR,
	SYSCALL_RMDIR,
	SYSCALL_MOUNT,
	SYSCALL_UNMOUNT,
	SYSCALL_WAITCHILD,
	SYSCALL_TELL,
	SYSCALL_GETCONF,
	SYSCALL_GETWORK,

	/* 40 */
	SYSCALL_JOIN,
	SYSCALL_FSTAT,
	SYSCALL_MMAP,
	SYSCALL_MPROTECT,
	SYSCALL_MUNMAP,
	SYSCALL_GETUID,
	SYSCALL_SETUID,
	SYSCALL_GETEUID,
	SYSCALL_SETEUID,
	SYSCALL_GETGID,

	/* 50 */
	SYSCALL_SETGID,
	SYSCALL_GETEGID,
	SYSCALL_SETEGID,
	SYSCALL_CHMOD,
	SYSCALL_CHOWN,
	SYSCALL_GETGROUPS,
	SYSCALL_SETGROUPS,
	SYSCALL_ISINGROUP,
	SYSCALL_ALARM,
	SYSCALL_TSCTOTIME,

	/* 60 */
	SYSCALL_SEMCRT,
	SYSCALL_SEMOP,
	SYSCALL_SEMDESTROY,
	SYSCALL_SENDRECV,
	SYSCALL_SHAREFILE,
	SYSCALL_CANCEL,
	SYSCALL_CREATSIBL,
	SYSCALL_GETCONFSTR,
	SYSCALL_CLONEMS,
	SYSCALL_JOINMS,

	/* 70 */
	SYSCALL_MLOCK,
	SYSCALL_MLOCKALL,
	SYSCALL_SEMCRTIRQ,
	SYSCALL_BINDTO,
	SYSCALL_RENAME,
	SYSCALL_GETTOD,
	SYSCALL_UTIME,
	SYSCALL_TRUNCATE,
#	ifdef __x86__
	SYSCALL_REQIOPORTS,
	SYSCALL_RELIOPORTS,
#	else
	SYSCALL_DEBUG,
#	endif
};

#	if defined(__i586__)
#		include <sys/arch/i586/syscalls.h>
#	elif defined(__x86_64__)
#		include <sys/arch/x86_64/syscalls.h>
#	elif defined(__eco32__)
#		include <sys/arch/eco32/syscalls.h>
#	else
#		include <sys/arch/mmix/syscalls.h>
#	endif
#endif

#if defined(__x86__)
#	define ASM_IRQ_ACKSIG				49
#endif
#define ASM_SYSC_ACKSIG					16

#if !defined(IN_ASM) && defined(__cplusplus)
static_assert(ASM_SYSC_ACKSIG == SYSCALL_ACKSIG,"ACKSIG is not equal");
#endif
