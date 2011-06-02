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

.global _changeSize
_changeSize:
	POP		0,0

.global _addRegion
_addRegion:
	POP		0,0

.global setRegProt
setRegProt:
	POP		0,0

.global _mapPhysical
_mapPhysical:
	POP		0,0

.global _createSharedMem
_createSharedMem:
	POP		0,0

.global _joinSharedMem
_joinSharedMem:
	POP		0,0

.global leaveSharedMem
leaveSharedMem:
	POP		0,0

.global destroySharedMem
destroySharedMem:
	POP		0,0

#SYSC_RET_1ARGS_ERR _changeSize,$SYSCALL_CHGSIZE
#SYSC_RET_5ARGS_ERR _addRegion,$SYSCALL_ADDREGION
#SYSC_RET_2ARGS_ERR setRegProt,$SYSCALL_SETREGPROT
#SYSC_RET_2ARGS_ERR _mapPhysical,$SYSCALL_MAPPHYS
#SYSC_RET_2ARGS_ERR _createSharedMem,$SYSCALL_CREATESHMEM
#SYSC_RET_1ARGS_ERR _joinSharedMem,$SYSCALL_JOINSHMEM
#SYSC_RET_1ARGS_ERR leaveSharedMem,$SYSCALL_LEAVESHMEM
#SYSC_RET_1ARGS_ERR destroySharedMem,$SYSCALL_DESTROYSHMEM
