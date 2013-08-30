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

.global _start
.global sigRetFunc

_start:
	# load modules first
	mov		$SYSCALL_LOADMODS,%eax
	mov		$1f,%edx
	mov		%esp,%ecx
	sysenter
1:

	# load vm86-task
	mov		$SYSCALL_VM86START,%eax
	mov		$1f,%edx
	mov		%esp,%ecx
	sysenter
1:

	# now replace with init
	mov		$progName,%esi						# arg1: path
	pushl	$args								# arg2: arguments
	mov		$SYSCALL_EXEC,%eax					# set syscall-number
	mov		$1f,%edx
	mov		%esp,%ecx
	sysenter
1:
	# we should not reach this
	jmp		1b

# provide just a dummy
sigRetFunc:
	jmp		sigRetFunc

.section .data

args:
	.long	progName,0

progName:
	.asciz	"/bin/init"
