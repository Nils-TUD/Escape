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

.include "syscalls.s"

.extern errno

SYSC_RET_2ARGS_ERR open,$SYSCALL_OPEN
SYSC_RET_2ARGS_ERR pipe,$SYSCALL_PIPE
SYSC_RET_2ARGS_ERR stat,$SYSCALL_STAT
SYSC_RET_2ARGS_ERR fstat,$SYSCALL_FSTAT
SYSC_RET_2ARGS_ERR tell,$SYSCALL_TELL
SYSC_RET_3ARGS_ERR fcntl,$SYSCALL_FCNTL
SYSC_RET_3ARGS_ERR seek,$SYSCALL_SEEK
SYSC_RET_3ARGS_ERR read,$SYSCALL_READ
SYSC_RET_3ARGS_ERR write,$SYSCALL_WRITE
SYSC_RET_1ARGS_ERR hasMsg,$SYSCALL_HASMSG
SYSC_RET_1ARGS_ERR isterm,$SYSCALL_ISTERM
SYSC_RET_4ARGS_ERR send,$SYSCALL_SEND
SYSC_RET_4ARGS_ERR receive,$SYSCALL_RECEIVE
SYSC_RET_1ARGS_ERR dupFd,$SYSCALL_DUPFD
SYSC_RET_2ARGS_ERR redirFd,$SYSCALL_REDIRFD
SYSC_RET_2ARGS_ERR link,$SYSCALL_LINK
SYSC_RET_1ARGS_ERR unlink,$SYSCALL_UNLINK
SYSC_RET_1ARGS_ERR mkdir,$SYSCALL_MKDIR
SYSC_RET_1ARGS_ERR rmdir,$SYSCALL_RMDIR
SYSC_RET_3ARGS_ERR mount,$SYSCALL_MOUNT
SYSC_RET_1ARGS_ERR unmount,$SYSCALL_UNMOUNT
SYSC_RET_0ARGS_ERR sync,$SYSCALL_SYNC
SYSC_VOID_1ARGS close,$SYSCALL_CLOSE
