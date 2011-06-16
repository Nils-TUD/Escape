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

.global _start
.global sigRetFunc
.extern main
.extern exit
.extern init_tls
.extern _init

.include "arch/mmix/syscalls.s"

# Initial software stack:
# +------------------+  <- top
# |     arguments    |
# |        ...       |
# +------------------+
#
# Registers:
# $1 = argc
# $2 = argv
# $4 = entryPoint (0 for initial thread, thread-entrypoint for others)
# $5 = TLSStart (0 if not present)
# $6 = TLSSize (0 if not present)

.ifndef SHAREDLIB
_start:
	LDOU	$0,$254,0
	UNSAVE	0,$0
	# call init_tls(entryPoint,TLSStart,TLSSize)
	PUSHJ	$3,init_tls
	# it returns the entrypoint; 0 if we're the initial thread
	BZ		$3,initialThread
	# we're an additional thread, so call the desired function
	PUSHGO	$0,$3,0
	JMP		threadExit

	# initial thread calls main
initialThread:
	.extern __libc_init
	PUSHJ	$3,__libc_init
	# call function in .init-section
	PUSHJ	$3,_init
	# finally, call main
	PUSHJ	$0,main

threadExit:
	SET		$1,$0
	PUSHJ	$0,exit
	# just to be sure
	1: JMP	1b

# all signal-handler return to this "function"
sigRetFunc:
	JMP		sigRetFunc
	#mov		$SYSCALL_ACKSIG,%eax
	#int		$SYSCALL_IRQ
	# never reached
.endif
