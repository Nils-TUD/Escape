#
# $Id: main.s 900 2011-06-02 20:18:17Z nasmussen $
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
	# load modules first; wait until we're finished ($0 = 0)
	SET		$7,SYSCALL_LOADMODS
	TRAP	1,0,0
	BNZ		$0,_start

	# now replace with init
	SET		$7,SYSCALL_EXEC
	GETA	$0,progName
	GETA	$1,args
	TRAP	1,0,0

	# we should not reach this
1:
	JMP		1b

# provide just a dummy
sigRetFunc:
	JMP		sigRetFunc

	LOC		@+(8-@)&7
args:
	OCTA	progName
	OCTA	0

progName:
	.asciz	"/bin/init"

# we need the stack-pointer and frame-pointer here for forking
sp GREG
fp GREG
