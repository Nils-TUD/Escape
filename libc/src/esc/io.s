;
; $Id$
; Copyright (C) 2008 - 2009 Nils Asmussen
;
; This program is free software; you can redistribute it and/or
; modify it under the terms of the GNU General Public License
; as published by the Free Software Foundation; either version 2
; of the License, or (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program; if not, write to the Free Software
; Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
;

[BITS 32]

%include "syscalls.s"

[extern errno]

SYSC_RET_2ARGS_ERR open,SYSCALL_OPEN
SYSC_RET_2ARGS_ERR getFileInfo,SYSCALL_STAT
SYSC_RET_1ARGS_ERR eof,SYSCALL_EOF
SYSC_RET_3ARGS_ERR seek,SYSCALL_SEEK
SYSC_RET_3ARGS_ERR read,SYSCALL_READ
SYSC_RET_3ARGS_ERR write,SYSCALL_WRITE
SYSC_RET_4ARGS_ERR ioctl,SYSCALL_IOCTL
SYSC_RET_4ARGS_ERR send,SYSCALL_SEND
SYSC_RET_4ARGS_ERR receive,SYSCALL_RECEIVE
SYSC_RET_1ARGS_ERR dupFd,SYSCALL_DUPFD
SYSC_RET_2ARGS_ERR redirFd,SYSCALL_REDIRFD
SYSC_RET_2ARGS_ERR link,SYSCALL_LINK
SYSC_RET_1ARGS_ERR unlink,SYSCALL_UNLINK
SYSC_RET_1ARGS_ERR mkdir,SYSCALL_MKDIR
SYSC_RET_1ARGS_ERR rmdir,SYSCALL_RMDIR
SYSC_RET_0ARGS_ERR sync,SYSCALL_SYNC
SYSC_VOID_1ARGS close,SYSCALL_CLOSE
