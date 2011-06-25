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

#include <esc/syscalls.h>
#include <esc/arch/i586/syscalls.s>

.section .text

.extern errno
.extern _GLOBAL_OFFSET_TABLE_

# needed for shared libraries to get the address of the GOT
.ifdef SHAREDLIB
# put in the linkonce-section to tell the linker that he should put this function in the
# executable / shared library just once
.section .gnu.linkonce.t.__i686.get_pc_thunk.bx,"ax",@progbits
.global __i686.get_pc_thunk.bx
.hidden  __i686.get_pc_thunk.bx
.type    __i686.get_pc_thunk.bx,@function
__i686.get_pc_thunk.bx:
	movl (%esp),%eax
	ret
.endif

# i586 specific syscalls
SYSC_RET_2ARGS_ERR requestIOPorts,SYSCALL_REQIOPORTS
SYSC_RET_2ARGS_ERR releaseIOPorts,SYSCALL_RELIOPORTS
SYSC_RET_4ARGS_ERR vm86int,SYSCALL_VM86INT
