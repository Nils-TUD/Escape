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

.section .text

.global _start
.global sigRetFunc

_start:
	# load modules first
	add		$4,$0,ASM_SYSC_LOADMODS
	trap

	# now replace with init
	add		$4,$0,ASM_SYSC_EXEC				# set syscall-number
	add		$5,$0,progName					# set path
	add		$6,$0,args						# set arguments
	trap

	# we should not reach this
1:
	j		1b

# provide just a dummy
sigRetFunc:
	j		sigRetFunc

args:
	.long	progName,0

progName:
	.asciz	"/bin/init"
