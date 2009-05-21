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

SYSC_RET_2ARGS_ERR regService,SYSCALL_REG
SYSC_RET_1ARGS_ERR unregService,SYSCALL_UNREG
SYSC_RET_3ARGS_ERR getClient,SYSCALL_GETCLIENT
SYSC_RET_2ARGS_ERR getClientThread,SYSCALL_GETCLIENTPROC
SYSC_RET_1ARGS_ERR createNode,SYSCALL_CREATENODE
