#
# $Id$
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

.include "arch/mmix/syscalls.s"

.extern errno

.global gettid
gettid:
	POP		0,0

.global getThreadCount
getThreadCount:
	POP		0,0

.global startThread
startThread:
	POP		0,0

.global _exit
_exit:
	POP		0,0

.global getCycles
getCycles:
	POP		0,0

.global yield
yield:
	POP		0,0

.global sleep
sleep:
	POP		0,0

.global waitm
waitm:
	POP		0,0

.global _waitUnlock
_waitUnlock:
	POP		0,0

.global notify
notify:
	POP		0,0

.global _lock
_lock:
	POP		0,0

.global _unlock
_unlock:
	POP		0,0

.global join
join:
	POP		0,0

.global suspend
suspend:
	POP		0,0

.global resume
resume:
	POP		0,0

#SYSC_RET_0ARGS gettid,$SYSCALL_GETTID
#SYSC_RET_0ARGS getThreadCount,$SYSCALL_GETTHREADCNT
#SYSC_RET_2ARGS_ERR startThread,$SYSCALL_STARTTHREAD
#SYSC_VOID_1ARGS _exit,$SYSCALL_EXIT
#SYSC_RET_0ARGS getCycles,$SYSCALL_GETCYCLES
#SYSC_VOID_0ARGS yield,$SYSCALL_YIELD
#SYSC_RET_1ARGS_ERR sleep,$SYSCALL_SLEEP
#SYSC_RET_2ARGS_ERR waitm,$SYSCALL_WAIT
#SYSC_RET_4ARGS_ERR _waitUnlock,$SYSCALL_WAITUNLOCK
#SYSC_RET_2ARGS_ERR notify,$SYSCALL_NOTIFY
#SYSC_RET_3ARGS_ERR _lock,$SYSCALL_LOCK
#SYSC_RET_2ARGS_ERR _unlock,$SYSCALL_UNLOCK
#SYSC_RET_1ARGS_ERR join,$SYSCALL_JOIN
#SYSC_RET_1ARGS_ERR suspend,$SYSCALL_SUSPEND
#SYSC_RET_1ARGS_ERR resume,$SYSCALL_RESUME
