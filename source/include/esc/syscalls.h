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

/* the syscall-numbers */
#define SYSCALL_PID				0
#define SYSCALL_PPID			1
#define SYSCALL_DEBUGCHAR		2
#define SYSCALL_FORK			3
#define SYSCALL_EXIT			4
#define SYSCALL_OPEN			5
#define SYSCALL_CLOSE			6
#define SYSCALL_READ			7
#define SYSCALL_CRTDEV			8
#define SYSCALL_CHGSIZE			9
#define SYSCALL_MAPPHYS			10
#define SYSCALL_WRITE			11
#define SYSCALL_YIELD			12
#define SYSCALL_DUPFD			13
#define SYSCALL_REDIRFD			14
#define SYSCALL_WAIT			15
#define SYSCALL_SETSIGH			16
#define SYSCALL_ACKSIG			17
#define SYSCALL_SENDSIG			18
#define SYSCALL_EXEC			19
#define SYSCALL_FCNTL			20
#define SYSCALL_LOADMODS		21
#define SYSCALL_SLEEP			22
#define SYSCALL_SEEK			23
#define SYSCALL_STAT			24
#define SYSCALL_DEBUG			25
#define SYSCALL_GETCLIENTPROC	26
#define SYSCALL_LOCK			27
#define SYSCALL_UNLOCK			28
#define SYSCALL_STARTTHREAD		29
#define SYSCALL_GETTID			30
#define SYSCALL_GETTHREADCNT	31
#define SYSCALL_SEND			32
#define SYSCALL_RECEIVE			33
#define SYSCALL_GETCYCLES		34
#define SYSCALL_SYNC			35
#define SYSCALL_LINK			36
#define SYSCALL_UNLINK			37
#define SYSCALL_MKDIR			38
#define SYSCALL_RMDIR			39
#define SYSCALL_MOUNT			40
#define SYSCALL_UNMOUNT			41
#define SYSCALL_WAITCHILD		42
#define SYSCALL_TELL			43
#define SYSCALL_PIPE			44
#define SYSCALL_GETCONF			45
#define SYSCALL_GETWORK			46
#define SYSCALL_JOIN			47
#define SYSCALL_SUSPEND			48
#define SYSCALL_RESUME			49
#define SYSCALL_FSTAT			50
#define SYSCALL_MMAP			51
#define SYSCALL_MPROTECT		52
#define SYSCALL_MUNMAP			53
#define SYSCALL_NOTIFY			54
#define SYSCALL_GETCLIENTID		55
#define SYSCALL_WAITUNLOCK		56
#define SYSCALL_GETENVITO		57
#define SYSCALL_GETENVTO		58
#define SYSCALL_SETENV			59
#define SYSCALL_GETUID			60
#define SYSCALL_SETUID			61
#define SYSCALL_GETEUID			62
#define SYSCALL_SETEUID			63
#define SYSCALL_GETGID			64
#define SYSCALL_SETGID			65
#define SYSCALL_GETEGID			66
#define SYSCALL_SETEGID			67
#define SYSCALL_CHMOD			68
#define SYSCALL_CHOWN			69
#define SYSCALL_GETGROUPS		70
#define SYSCALL_SETGROUPS		71
#define SYSCALL_ISINGROUP		72
#define SYSCALL_ALARM			73
#define SYSCALL_TSCTOTIME		74
#define SYSCALL_REQIOPORTS		75
#define SYSCALL_RELIOPORTS		76
#define SYSCALL_VM86INT			77
#define SYSCALL_VM86START		78
