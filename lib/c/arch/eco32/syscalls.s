#
# $Id: syscalls.s 833 2010-10-02 14:17:28Z nasmussen $
# Copyright (C) 2008 - 2009 Nils Asmussen
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#

.section .text

.extern _GLOBAL_OFFSET_TABLE_

# the syscall-numbers
.set SYSCALL_PID,						0
.set SYSCALL_PPID,					1
.set SYSCALL_DEBUGCHAR,			2
.set SYSCALL_FORK,					3
.set SYSCALL_EXIT,					4
.set SYSCALL_OPEN,					5
.set SYSCALL_CLOSE,					6
.set SYSCALL_READ,					7
.set SYSCALL_REG,						8
.set SYSCALL_CHGSIZE,				9
.set SYSCALL_MAPPHYS,				10
.set SYSCALL_WRITE,					11
.set SYSCALL_YIELD,					12
.set SYSCALL_REQIOPORTS,		13
.set SYSCALL_RELIOPORTS,		14
.set SYSCALL_DUPFD,					15
.set SYSCALL_REDIRFD,				16
.set SYSCALL_WAIT,					17
.set SYSCALL_SETSIGH,				18
.set SYSCALL_ACKSIG,				19
.set SYSCALL_SENDSIG,				20
.set SYSCALL_EXEC,					21
.set SYSCALL_FCNTL,					22
.set SYSCALL_LOADMODS,			23
.set SYSCALL_SLEEP,					24
.set SYSCALL_SEEK,					25
.set SYSCALL_STAT,					26
.set SYSCALL_DEBUG,					27
.set SYSCALL_CREATESHMEM,		28
.set SYSCALL_JOINSHMEM,			29
.set SYSCALL_LEAVESHMEM,		30
.set SYSCALL_DESTROYSHMEM,	31
.set SYSCALL_GETCLIENTPROC,	32
.set SYSCALL_LOCK,					33
.set SYSCALL_UNLOCK,				34
.set SYSCALL_STARTTHREAD,		35
.set SYSCALL_GETTID,				36
.set SYSCALL_GETTHREADCNT,	37
.set SYSCALL_SEND,					38
.set SYSCALL_RECEIVE,				39
.set SYSCALL_GETCYCLES,			40
.set SYSCALL_SYNC,					41
.set SYSCALL_LINK,					42
.set SYSCALL_UNLINK,				43
.set SYSCALL_MKDIR,					44
.set SYSCALL_RMDIR,					45
.set SYSCALL_MOUNT,					46
.set SYSCALL_UNMOUNT,				47
.set SYSCALL_WAITCHILD,			48
.set SYSCALL_TELL,					49
.set SYSCALL_PIPE,					50
.set SYSCALL_GETCONF,				51
.set SYSCALL_VM86INT,				52
.set SYSCALL_GETWORK,				53
.set SYSCALL_ISTERM,				54
.set SYSCALL_JOIN,					55
.set SYSCALL_SUSPEND,				56
.set SYSCALL_RESUME,				57
.set SYSCALL_FSTAT,					58
.set SYSCALL_ADDREGION,			59
.set SYSCALL_SETREGPROT,		60
.set SYSCALL_NOTIFY,				61
.set SYSCALL_GETCLIENTID,		62
.set SYSCALL_WAITUNLOCK,		63
.set SYSCALL_GETENVITO,			64
.set SYSCALL_GETENVTO,			65
.set SYSCALL_SETENV,				66

# the IRQ for syscalls
.set SYSCALL_IRQ,						0x30

# macros for the different syscall-types (void / ret-value, arg-count, error-handling)

.macro SYSC_VOID_0ARGS name syscno
.global \name
.type \name, @function
\name:
	jr		$31
#	mov		\syscno,%eax					# set syscall-number
#	int		$SYSCALL_IRQ
#	ret
.endm

.macro SYSC_VOID_1ARGS name syscno
.global \name
.type \name, @function
\name:
	jr		$31
#	mov		\syscno,%eax					# set syscall-number
#	mov		4(%esp),%ecx					# set arg1
#	int		$SYSCALL_IRQ
#	ret
.endm

.macro SYSC_RET_0ARGS name syscno
.global \name
.type \name, @function
\name:
	jr		$31
#	mov		\syscno,%eax					# set syscall-number
#	int		$SYSCALL_IRQ
#	ret													# return-value is in eax
.endm

.macro SYSC_RET_0ARGS_ERR name syscno
.global \name
.type \name, @function
\name:
	jr		$31
#	mov		\syscno,%eax					# set syscall-number
#	int		$SYSCALL_IRQ
#	test	%ecx,%ecx
#	jz		1f										# no-error?
#	STORE_ERRNO
#	mov		%ecx,%eax							# return error-code
#1:
#	ret
.endm

.macro SYSC_RET_1ARGS_ERR name syscno
.global \name
.type \name, @function
\name:
	jr		$31
#	mov		\syscno,%eax					# set syscall-number
#	mov		4(%esp),%ecx					# set arg1
#	int		$SYSCALL_IRQ
#	test	%ecx,%ecx
#	jz		1f										# no-error?
#	STORE_ERRNO
#	mov		%ecx,%eax							# return error-code
#1:
#	ret
.endm

.macro SYSC_RET_2ARGS_ERR name syscno
.global \name
.type \name, @function
\name:
	jr		$31
#	mov		\syscno,%eax					# set syscall-number
#	mov		4(%esp),%ecx					# set arg1
#	mov		8(%esp),%edx					# set arg2
#	int		$SYSCALL_IRQ
#	test	%ecx,%ecx
#	jz		1f										# no-error?
#	STORE_ERRNO
#	mov		%ecx,%eax							# return error-code
#1:
#	ret
.endm

.macro SYSC_RET_3ARGS_ERR name syscno
.global \name
.type \name, @function
\name:
	jr		$31
#	push	%ebp
#	mov		%esp,%ebp
#	mov		8(%ebp),%ecx					# set arg1
#	mov		12(%ebp),%edx					# set arg2
#	pushl	16(%ebp)							# push arg3
#	mov		\syscno,%eax					# set syscall-number
#	int		$SYSCALL_IRQ
#	add		$4,%esp								# remove arg3
#	test	%ecx,%ecx
#	jz		1f										# no-error?
#	STORE_ERRNO
#	mov		%ecx,%eax							# return error-code
#1:
#	leave
#	ret
.endm

.macro SYSC_RET_4ARGS_ERR name syscno
.global \name
.type \name, @function
\name:
	jr		$31
#	push	%ebp
#	mov		%esp,%ebp
#	mov		8(%ebp),%ecx					# set arg1
#	mov		12(%ebp),%edx					# set arg2
#	pushl	20(%ebp)							# push arg4
#	pushl	16(%ebp)							# push arg3
#	mov		\syscno,%eax					# set syscall-number
#	int		$SYSCALL_IRQ
#	add		$8,%esp								# remove arg3 and arg4
#	test	%ecx,%ecx
#	jz		1f										# no-error?
#	STORE_ERRNO
#	mov		%ecx,%eax							# return error-code
#1:
#	leave
#	ret
.endm

.macro SYSC_RET_5ARGS_ERR name syscno
.global \name
.type \name, @function
\name:
	jr		$31
#	push	%ebp
#	mov		%esp,%ebp
#	mov		8(%ebp),%ecx					# set arg1
#	mov		12(%ebp),%edx					# set arg2
#	pushl	24(%ebp)							# push arg5
#	pushl	20(%ebp)							# push arg4
#	pushl	16(%ebp)							# push arg3
#	mov		\syscno,%eax					# set syscall-number
#	int		$SYSCALL_IRQ
#	add		$12,%esp							# remove arg3, arg4 and arg5
#	test	%ecx,%ecx
#	jz		1f										# no-error?
#	STORE_ERRNO
#	mov		%ecx,%eax							# return error-code
#1:
#	leave
#	ret
.endm

.macro SYSC_RET_7ARGS_ERR name syscno
.global \name
.type \name, @function
\name:
	jr		$31
#	push	%ebp
#	mov		%esp,%ebp
#	mov		8(%ebp),%ecx					# set arg1
#	mov		12(%ebp),%edx					# set arg2
#	pushl	32(%ebp)							# push arg7
#	pushl	28(%ebp)							# push arg6
#	pushl	24(%ebp)							# push arg5
#	pushl	20(%ebp)							# push arg4
#	pushl	16(%ebp)							# push arg3
#	mov		\syscno,%eax					# set syscall-number
#	int		$SYSCALL_IRQ
#	add		$20,%esp							# remove args
#	test	%ecx,%ecx
#	jz		1f										# no-error?
#	STORE_ERRNO
#	mov		%ecx,%eax							# return error-code
#1:
#	leave
#	ret
.endm

# stores the received error-code (in ecx) to the global variable errno
.macro STORE_ERRNO
	#mov		%ecx,(errno)					# store error-code
.endm
