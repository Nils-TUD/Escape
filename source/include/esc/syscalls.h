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
#define SYSCALL_CREATESHMEM		26
#define SYSCALL_JOINSHMEM		27
#define SYSCALL_LEAVESHMEM		28
#define SYSCALL_DESTROYSHMEM	29
#define SYSCALL_GETCLIENTPROC	30
#define SYSCALL_LOCK			31
#define SYSCALL_UNLOCK			32
#define SYSCALL_STARTTHREAD		33
#define SYSCALL_GETTID			34
#define SYSCALL_GETTHREADCNT	35
#define SYSCALL_SEND			36
#define SYSCALL_RECEIVE			37
#define SYSCALL_GETCYCLES		38
#define SYSCALL_SYNC			39
#define SYSCALL_LINK			40
#define SYSCALL_UNLINK			41
#define SYSCALL_MKDIR			42
#define SYSCALL_RMDIR			43
#define SYSCALL_MOUNT			44
#define SYSCALL_UNMOUNT			45
#define SYSCALL_WAITCHILD		46
#define SYSCALL_TELL			47
#define SYSCALL_PIPE			48
#define SYSCALL_GETCONF			49
#define SYSCALL_GETWORK			50
#define SYSCALL_JOIN			51
#define SYSCALL_SUSPEND			52
#define SYSCALL_RESUME			53
#define SYSCALL_FSTAT			54
#define SYSCALL_ADDREGION		55
#define SYSCALL_SETREGPROT		56
#define SYSCALL_NOTIFY			57
#define SYSCALL_GETCLIENTID		58
#define SYSCALL_WAITUNLOCK		59
#define SYSCALL_GETENVITO		60
#define SYSCALL_GETENVTO		61
#define SYSCALL_SETENV			62
#define SYSCALL_GETUID			63
#define SYSCALL_SETUID			64
#define SYSCALL_GETEUID			65
#define SYSCALL_SETEUID			66
#define SYSCALL_GETGID			67
#define SYSCALL_SETGID			68
#define SYSCALL_GETEGID			69
#define SYSCALL_SETEGID			70
#define SYSCALL_CHMOD			71
#define SYSCALL_CHOWN			72
#define SYSCALL_GETGROUPS		73
#define SYSCALL_SETGROUPS		74
#define SYSCALL_ISINGROUP		75
#define SYSCALL_ALARM			76
#define SYSCALL_MAPMOD			77
#define SYSCALL_REQIOPORTS		78
#define SYSCALL_RELIOPORTS		79
#define SYSCALL_VM86INT			80
#define SYSCALL_VM86START		81
