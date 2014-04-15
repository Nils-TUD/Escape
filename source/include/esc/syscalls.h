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
	SYSCALL_LOADMODS,
	SYSCALL_SLEEP,
	SYSCALL_SEEK,
	SYSCALL_STAT,
	SYSCALL_STARTTHREAD,
	SYSCALL_GETTID,
	SYSCALL_GETTHREADCNT,
	SYSCALL_SEND,
	SYSCALL_RECEIVE,
	SYSCALL_GETCYCLES,

	/* 30 */
	SYSCALL_SYNCFS,
	SYSCALL_LINK,
	SYSCALL_UNLINK,
	SYSCALL_MKDIR,
	SYSCALL_RMDIR,
	SYSCALL_MOUNT,
	SYSCALL_UNMOUNT,
	SYSCALL_WAITCHILD,
	SYSCALL_TELL,
	SYSCALL_PIPE,

	/* 40 */
	SYSCALL_GETCONF,
	SYSCALL_GETWORK,
	SYSCALL_JOIN,
	SYSCALL_FSTAT,
	SYSCALL_MMAP,
	SYSCALL_MPROTECT,
	SYSCALL_MUNMAP,
	SYSCALL_GETENVITO,
	SYSCALL_GETENVTO,
	SYSCALL_SETENV,

	/* 50 */
	SYSCALL_GETUID,
	SYSCALL_SETUID,
	SYSCALL_GETEUID,
	SYSCALL_SETEUID,
	SYSCALL_GETGID,
	SYSCALL_SETGID,
	SYSCALL_GETEGID,
	SYSCALL_SETEGID,
	SYSCALL_CHMOD,
	SYSCALL_CHOWN,

	/* 60 */
	SYSCALL_GETGROUPS,
	SYSCALL_SETGROUPS,
	SYSCALL_ISINGROUP,
	SYSCALL_ALARM,
	SYSCALL_TSCTOTIME,
	SYSCALL_SEMCRT,
	SYSCALL_SEMOP,
	SYSCALL_SEMDESTROY,
	SYSCALL_SENDRECV,
	SYSCALL_SHAREFILE,

	/* 70 */
	SYSCALL_CANCEL,
	SYSCALL_GETCONFSTR,
	SYSCALL_GETMSID,
	SYSCALL_CLONEMS,
	SYSCALL_JOINMS,
	SYSCALL_MLOCK,
	SYSCALL_MLOCKALL,
	SYSCALL_SEMCRTIRQ,
#	ifdef __i386__
	SYSCALL_REQIOPORTS,
	SYSCALL_RELIOPORTS,
	SYSCALL_VM86INT,
	SYSCALL_VM86START,
#	else
	SYSCALL_DEBUG,
#	endif
};

#	if defined(__i386__)
#		include <esc/arch/i586/syscalls.h>
#	elif defined(__eco32__)
#		include <esc/arch/eco32/syscalls.h>
#	else
#		include <esc/arch/mmix/syscalls.h>
#	endif
#endif

#ifdef __i386__
#	define ASM_IRQ_ACKSIG				49
#endif
#define ASM_SYSC_ACKSIG					16
#define ASM_SYSC_EXEC					18
#define ASM_SYSC_LOADMODS				20
#define ASM_SYSC_VM86START				81

#if !defined(IN_ASM) && defined(__cplusplus)
static_assert(ASM_SYSC_ACKSIG == SYSCALL_ACKSIG,"ACKSIG is not equal");
static_assert(ASM_SYSC_EXEC == SYSCALL_EXEC,"ACKSIG is not equal");
static_assert(ASM_SYSC_LOADMODS == SYSCALL_LOADMODS,"ACKSIG is not equal");
#	ifdef __i386__
static_assert(ASM_SYSC_VM86START == SYSCALL_VM86START,"ACKSIG is not equal");
#	endif
#endif
