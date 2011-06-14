#
# $Id: syscalls.s 900 2011-06-02 20:18:17Z nasmussen $
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

.global getConf
.type getConf, @function
getConf:
	SET		$7,SYSCALL_GETCONF				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global debugChar
.type debugChar, @function
debugChar:
	SET		$7,SYSCALL_DEBUGCHAR				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global debug
.type debug, @function
debug:
	SET		$7,SYSCALL_DEBUG				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global regDriver
.type regDriver, @function
regDriver:
	SET		$7,SYSCALL_REG				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global getClientId
.type getClientId, @function
getClientId:
	SET		$7,SYSCALL_GETCLIENTID				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global getClient
.type getClient, @function
getClient:
	SET		$7,SYSCALL_GETCLIENTPROC				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global getWork
.type getWork, @function
getWork:
	SET		$7,SYSCALL_GETWORK				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global open
.type open, @function
open:
	SET		$7,SYSCALL_OPEN				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global pipe
.type pipe, @function
pipe:
	SET		$7,SYSCALL_PIPE				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global stat
.type stat, @function
stat:
	SET		$7,SYSCALL_STAT				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global fstat
.type fstat, @function
fstat:
	SET		$7,SYSCALL_FSTAT				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global tell
.type tell, @function
tell:
	SET		$7,SYSCALL_TELL				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global fcntl
.type fcntl, @function
fcntl:
	SET		$7,SYSCALL_FCNTL				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global seek
.type seek, @function
seek:
	SET		$7,SYSCALL_SEEK				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global read
.type read, @function
read:
	SET		$7,SYSCALL_READ				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global write
.type write, @function
write:
	SET		$7,SYSCALL_WRITE				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global isterm
.type isterm, @function
isterm:
	SET		$7,SYSCALL_ISTERM				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global send
.type send, @function
send:
	SET		$7,SYSCALL_SEND				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global receive
.type receive, @function
receive:
	SET		$7,SYSCALL_RECEIVE				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global dupFd
.type dupFd, @function
dupFd:
	SET		$7,SYSCALL_DUPFD				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global redirFd
.type redirFd, @function
redirFd:
	SET		$7,SYSCALL_REDIRFD				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global link
.type link, @function
link:
	SET		$7,SYSCALL_LINK				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global unlink
.type unlink, @function
unlink:
	SET		$7,SYSCALL_UNLINK				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global mkdir
.type mkdir, @function
mkdir:
	SET		$7,SYSCALL_MKDIR				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global rmdir
.type rmdir, @function
rmdir:
	SET		$7,SYSCALL_RMDIR				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global mount
.type mount, @function
mount:
	SET		$7,SYSCALL_MOUNT				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global unmount
.type unmount, @function
unmount:
	SET		$7,SYSCALL_UNMOUNT				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global sync
.type sync, @function
sync:
	SET		$7,SYSCALL_SYNC				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global close
.type close, @function
close:
	SET		$7,SYSCALL_CLOSE				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global _changeSize
.type _changeSize, @function
_changeSize:
	SET		$7,SYSCALL_CHGSIZE				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global _addRegion
.type _addRegion, @function
_addRegion:
	SET		$7,SYSCALL_ADDREGION				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global setRegProt
.type setRegProt, @function
setRegProt:
	SET		$7,SYSCALL_SETREGPROT				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global _mapPhysical
.type _mapPhysical, @function
_mapPhysical:
	SET		$7,SYSCALL_MAPPHYS				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global _createSharedMem
.type _createSharedMem, @function
_createSharedMem:
	SET		$7,SYSCALL_CREATESHMEM				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global _joinSharedMem
.type _joinSharedMem, @function
_joinSharedMem:
	SET		$7,SYSCALL_JOINSHMEM				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global leaveSharedMem
.type leaveSharedMem, @function
leaveSharedMem:
	SET		$7,SYSCALL_LEAVESHMEM				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global destroySharedMem
.type destroySharedMem, @function
destroySharedMem:
	SET		$7,SYSCALL_DESTROYSHMEM				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global getpid
.type getpid, @function
getpid:
	SET		$7,SYSCALL_PID				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global getppidof
.type getppidof, @function
getppidof:
	SET		$7,SYSCALL_PPID				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global fork
.type fork, @function
fork:
	SET		$7,SYSCALL_FORK				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global exec
.type exec, @function
exec:
	SET		$7,SYSCALL_EXEC				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global waitChild
.type waitChild, @function
waitChild:
	SET		$7,SYSCALL_WAITCHILD				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global getenvito
.type getenvito, @function
getenvito:
	SET		$7,SYSCALL_GETENVITO				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global getenvto
.type getenvto, @function
getenvto:
	SET		$7,SYSCALL_GETENVTO				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global setenv
.type setenv, @function
setenv:
	SET		$7,SYSCALL_SETENV				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global setSigHandler
.type setSigHandler, @function
setSigHandler:
	SET		$7,SYSCALL_SETSIGH				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global sendSignalTo
.type sendSignalTo, @function
sendSignalTo:
	SET		$7,SYSCALL_SENDSIG				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global gettid
.type gettid, @function
gettid:
	SET		$7,SYSCALL_GETTID				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global getThreadCount
.type getThreadCount, @function
getThreadCount:
	SET		$7,SYSCALL_GETTHREADCNT				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global startThread
.type startThread, @function
startThread:
	SET		$7,SYSCALL_STARTTHREAD				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global _exit
.type _exit, @function
_exit:
	SET		$7,SYSCALL_EXIT				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global getCycles
.type getCycles, @function
getCycles:
	SET		$7,SYSCALL_GETCYCLES				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global yield
.type yield, @function
yield:
	SET		$7,SYSCALL_YIELD				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global sleep
.type sleep, @function
sleep:
	SET		$7,SYSCALL_SLEEP				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global waitm
.type waitm, @function
waitm:
	SET		$7,SYSCALL_WAIT				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global _waitUnlock
.type _waitUnlock, @function
_waitUnlock:
	SET		$7,SYSCALL_WAITUNLOCK				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global notify
.type notify, @function
notify:
	SET		$7,SYSCALL_NOTIFY				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global _lock
.type _lock, @function
_lock:
	SET		$7,SYSCALL_LOCK				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global _unlock
.type _unlock, @function
_unlock:
	SET		$7,SYSCALL_UNLOCK				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global join
.type join, @function
join:
	SET		$7,SYSCALL_JOIN				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global suspend
.type suspend, @function
suspend:
	SET		$7,SYSCALL_SUSPEND				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0

.global resume
.type resume, @function
resume:
	SET		$7,SYSCALL_RESUME				# set syscall-number
	TRAP	1,0,0
	BZ		$1,1f						# no-error?
	GETA	$2,errno
	STOU	$1,$2,0
	SET		$0,$1
1:
	POP		1,0							# return value is in $0
