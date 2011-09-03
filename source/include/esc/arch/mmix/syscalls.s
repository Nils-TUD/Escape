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

.global getConf
.type getConf, @function
getConf:
	SET		$7,0
	TRAP	0,SYSCALL_GETCONF,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global debugChar
.type debugChar, @function
debugChar:
	SET		$7,0
	TRAP	0,SYSCALL_DEBUGCHAR,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global debug
.type debug, @function
debug:
	SET		$7,0
	TRAP	0,SYSCALL_DEBUG,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global regDriver
.type regDriver, @function
regDriver:
	SET		$7,0
	TRAP	0,SYSCALL_REG,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global getClientId
.type getClientId, @function
getClientId:
	SET		$7,0
	TRAP	0,SYSCALL_GETCLIENTID,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global getClient
.type getClient, @function
getClient:
	SET		$7,0
	TRAP	0,SYSCALL_GETCLIENTPROC,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global getWork
.type getWork, @function
getWork:
	SET		$7,0
	TRAP	0,SYSCALL_GETWORK,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global open
.type open, @function
open:
	SET		$7,0
	TRAP	0,SYSCALL_OPEN,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global pipe
.type pipe, @function
pipe:
	SET		$7,0
	TRAP	0,SYSCALL_PIPE,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global stat
.type stat, @function
stat:
	SET		$7,0
	TRAP	0,SYSCALL_STAT,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global fstat
.type fstat, @function
fstat:
	SET		$7,0
	TRAP	0,SYSCALL_FSTAT,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global chmod
.type chmod, @function
chmod:
	SET		$7,0
	TRAP	0,SYSCALL_CHMOD,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global chown
.type chown, @function
chown:
	SET		$7,0
	TRAP	0,SYSCALL_CHOWN,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global tell
.type tell, @function
tell:
	SET		$7,0
	TRAP	0,SYSCALL_TELL,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global fcntl
.type fcntl, @function
fcntl:
	SET		$7,0
	TRAP	0,SYSCALL_FCNTL,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global seek
.type seek, @function
seek:
	SET		$7,0
	TRAP	0,SYSCALL_SEEK,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global read
.type read, @function
read:
	SET		$7,0
	TRAP	0,SYSCALL_READ,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global write
.type write, @function
write:
	SET		$7,0
	TRAP	0,SYSCALL_WRITE,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global send
.type send, @function
send:
	SET		$7,0
	TRAP	0,SYSCALL_SEND,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global receive
.type receive, @function
receive:
	SET		$7,0
	TRAP	0,SYSCALL_RECEIVE,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global dupFd
.type dupFd, @function
dupFd:
	SET		$7,0
	TRAP	0,SYSCALL_DUPFD,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global redirFd
.type redirFd, @function
redirFd:
	SET		$7,0
	TRAP	0,SYSCALL_REDIRFD,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global link
.type link, @function
link:
	SET		$7,0
	TRAP	0,SYSCALL_LINK,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global unlink
.type unlink, @function
unlink:
	SET		$7,0
	TRAP	0,SYSCALL_UNLINK,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global mkdir
.type mkdir, @function
mkdir:
	SET		$7,0
	TRAP	0,SYSCALL_MKDIR,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global rmdir
.type rmdir, @function
rmdir:
	SET		$7,0
	TRAP	0,SYSCALL_RMDIR,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global mount
.type mount, @function
mount:
	SET		$7,0
	TRAP	0,SYSCALL_MOUNT,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global unmount
.type unmount, @function
unmount:
	SET		$7,0
	TRAP	0,SYSCALL_UNMOUNT,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global sync
.type sync, @function
sync:
	SET		$7,0
	TRAP	0,SYSCALL_SYNC,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global close
.type close, @function
close:
	SET		$7,0
	TRAP	0,SYSCALL_CLOSE,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global _changeSize
.type _changeSize, @function
_changeSize:
	SET		$7,0
	TRAP	0,SYSCALL_CHGSIZE,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global _addRegion
.type _addRegion, @function
_addRegion:
	SET		$7,0
	TRAP	0,SYSCALL_ADDREGION,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global setRegProt
.type setRegProt, @function
setRegProt:
	SET		$7,0
	TRAP	0,SYSCALL_SETREGPROT,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global _mapPhysical
.type _mapPhysical, @function
_mapPhysical:
	SET		$7,0
	TRAP	0,SYSCALL_MAPPHYS,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global _createSharedMem
.type _createSharedMem, @function
_createSharedMem:
	SET		$7,0
	TRAP	0,SYSCALL_CREATESHMEM,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global _joinSharedMem
.type _joinSharedMem, @function
_joinSharedMem:
	SET		$7,0
	TRAP	0,SYSCALL_JOINSHMEM,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global leaveSharedMem
.type leaveSharedMem, @function
leaveSharedMem:
	SET		$7,0
	TRAP	0,SYSCALL_LEAVESHMEM,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global destroySharedMem
.type destroySharedMem, @function
destroySharedMem:
	SET		$7,0
	TRAP	0,SYSCALL_DESTROYSHMEM,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global getpid
.type getpid, @function
getpid:
	SET		$7,0
	TRAP	0,SYSCALL_PID,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global getppidof
.type getppidof, @function
getppidof:
	SET		$7,0
	TRAP	0,SYSCALL_PPID,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global getuid
.type getuid, @function
getuid:
	SET		$7,0
	TRAP	0,SYSCALL_GETUID,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global setuid
.type setuid, @function
setuid:
	SET		$7,0
	TRAP	0,SYSCALL_SETUID,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global geteuid
.type geteuid, @function
geteuid:
	SET		$7,0
	TRAP	0,SYSCALL_GETEUID,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global seteuid
.type seteuid, @function
seteuid:
	SET		$7,0
	TRAP	0,SYSCALL_SETEUID,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global getgid
.type getgid, @function
getgid:
	SET		$7,0
	TRAP	0,SYSCALL_GETGID,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global setgid
.type setgid, @function
setgid:
	SET		$7,0
	TRAP	0,SYSCALL_SETGID,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global getegid
.type getegid, @function
getegid:
	SET		$7,0
	TRAP	0,SYSCALL_GETEGID,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global setegid
.type setegid, @function
setegid:
	SET		$7,0
	TRAP	0,SYSCALL_SETEGID,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global getgroups
.type getgroups, @function
getgroups:
	SET		$7,0
	TRAP	0,SYSCALL_GETGROUPS,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global setgroups
.type setgroups, @function
setgroups:
	SET		$7,0
	TRAP	0,SYSCALL_SETGROUPS,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global isingroup
.type isingroup, @function
isingroup:
	SET		$7,0
	TRAP	0,SYSCALL_ISINGROUP,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global fork
.type fork, @function
fork:
	SET		$7,0
	TRAP	0,SYSCALL_FORK,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global exec
.type exec, @function
exec:
	SET		$7,0
	TRAP	0,SYSCALL_EXEC,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global waitChild
.type waitChild, @function
waitChild:
	SET		$7,0
	TRAP	0,SYSCALL_WAITCHILD,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global getenvito
.type getenvito, @function
getenvito:
	SET		$7,0
	TRAP	0,SYSCALL_GETENVITO,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global getenvto
.type getenvto, @function
getenvto:
	SET		$7,0
	TRAP	0,SYSCALL_GETENVTO,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global setenv
.type setenv, @function
setenv:
	SET		$7,0
	TRAP	0,SYSCALL_SETENV,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global setSigHandler
.type setSigHandler, @function
setSigHandler:
	SET		$7,0
	TRAP	0,SYSCALL_SETSIGH,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global sendSignalTo
.type sendSignalTo, @function
sendSignalTo:
	SET		$7,0
	TRAP	0,SYSCALL_SENDSIG,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global gettid
.type gettid, @function
gettid:
	SET		$7,0
	TRAP	0,SYSCALL_GETTID,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global getThreadCount
.type getThreadCount, @function
getThreadCount:
	SET		$7,0
	TRAP	0,SYSCALL_GETTHREADCNT,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global startThread
.type startThread, @function
startThread:
	SET		$7,0
	TRAP	0,SYSCALL_STARTTHREAD,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global _exit
.type _exit, @function
_exit:
	SET		$7,0
	TRAP	0,SYSCALL_EXIT,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global getCycles
.type getCycles, @function
getCycles:
	SET		$7,0
	TRAP	0,SYSCALL_GETCYCLES,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global yield
.type yield, @function
yield:
	SET		$7,0
	TRAP	0,SYSCALL_YIELD,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global alarm
.type alarm, @function
alarm:
	SET		$7,0
	TRAP	0,SYSCALL_ALARM,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global sleep
.type sleep, @function
sleep:
	SET		$7,0
	TRAP	0,SYSCALL_SLEEP,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global waitmUntil
.type waitmUntil, @function
waitmUntil:
	SET		$7,0
	TRAP	0,SYSCALL_WAIT,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global _waitUnlock
.type _waitUnlock, @function
_waitUnlock:
	SET		$7,0
	TRAP	0,SYSCALL_WAITUNLOCK,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global notify
.type notify, @function
notify:
	SET		$7,0
	TRAP	0,SYSCALL_NOTIFY,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global _lock
.type _lock, @function
_lock:
	SET		$7,0
	TRAP	0,SYSCALL_LOCK,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global _unlock
.type _unlock, @function
_unlock:
	SET		$7,0
	TRAP	0,SYSCALL_UNLOCK,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global join
.type join, @function
join:
	SET		$7,0
	TRAP	0,SYSCALL_JOIN,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global suspend
.type suspend, @function
suspend:
	SET		$7,0
	TRAP	0,SYSCALL_SUSPEND,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0

.global resume
.type resume, @function
resume:
	SET		$7,0
	TRAP	0,SYSCALL_RESUME,0
	BZ		$2,1f						# no-error?
	GETA	$3,errno
	STTU	$2,$3,0
	SET		$0,$2
1:
	POP		1,0							# return value is in $0
