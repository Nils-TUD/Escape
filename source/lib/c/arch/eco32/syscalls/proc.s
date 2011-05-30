#
# $Id: proc.s 815 2010-09-25 23:50:41Z nasmussen $
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

.include "arch/eco32/syscalls.s"

.extern errno

SYSC_RET_0ARGS getpid,SYSCALL_PID
SYSC_RET_1ARGS_ERR getppidof,SYSCALL_PPID
SYSC_RET_0ARGS_ERR fork,SYSCALL_FORK
SYSC_RET_2ARGS_ERR exec,SYSCALL_EXEC
SYSC_RET_1ARGS_ERR waitChild,SYSCALL_WAITCHILD
SYSC_RET_3ARGS_ERR getenvito,SYSCALL_GETENVITO
SYSC_RET_3ARGS_ERR getenvto,SYSCALL_GETENVTO
SYSC_RET_2ARGS_ERR setenv,SYSCALL_SETENV
