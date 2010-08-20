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

.global sigRetFunc
.global _start
.global lookup_resolveStart
.extern __libc_init
.extern lookup_resolve
.extern load_setupProg
.extern load_regFrameInfo

.include "../../../lib/c/syscalls.s"

#  Initial stack:
#  +------------------+  <- top
#  |     arguments    |
#  |        ...       |
#  +------------------+
#  |       argv       |
#  +------------------+
#  |       argc       |
#  +------------------+
#  |     TLSSize      |  0 if not present
#  +------------------+
#  |     TLSStart     |  0 if not present
#  +------------------+
#  |    entryPoint    |  0 for initial thread, thread-entrypoint for others
#  +------------------+
#  |    fd for prog   |
#  +------------------+

_start:
	# we have no TLS, so don't waste time
	push	$0
	# call __libc_init(0); no exceptions here
	call	__libc_init
	add		$4,%esp
	call	load_setupProg
	# first, remove fd from stack
	add		$4,%esp
	# push the address of load_regFrameInfo on the stack to pass it to crt0.s
	push	$load_regFrameInfo
	# load_setupProg returns the entrypoint
	jmp		*%eax

lookup_resolveStart:
.ifndef CALLTRACE_PID
	call	lookup_resolve
	add		$8,%esp												# remove args of lookup_resolve from stack
	jmp		*%eax													# jump to function
.else
	push	8(%esp)												# push ret-addr for the call
	call	lookup_resolve
	add		$12,%esp											# remove args of lookup_resolve from stack
	push	%eax
	call	getpid												# get pid
	cmp		$CALLTRACE_PID,%eax						# if not the requested pid skip the whole stuff
	pop		%eax
	jne		1f
	mov		(lookupDepth),%ecx
	cmp		$100,%ecx											# we have only 100 places...
	jae		1f
	mov		$lookupStack,%edx
	lea		(%edx,%ecx,4),%edx						# load addr to store ret-addr at
	add		$1,%ecx												# inc depth
	mov		%ecx,(lookupDepth)						# store
	mov		(%esp),%ecx										# get real return-addr
	mov		%ecx,(%edx)										# save
	mov		$lookup_resolveFinish,%ecx		# replace with our own
	mov		%ecx,(%esp)
1:
	jmp		*%eax													# jump to function

lookup_resolveFinish:
	.extern lookup_tracePop
	push	%eax
	call	lookup_tracePop
	pop		%eax
	mov		(lookupDepth),%ecx						# load depth
	sub		$1,%ecx												# decrement
	mov		%ecx,(lookupDepth)						# store
	mov		$lookupStack,%edx							# get stack
	lea		(%edx,%ecx,4),%edx						# build addr to our ret-addr
	mov		(%edx),%ecx										# get ret-addr
	jmp		*%ecx													# jump to that addr
.endif

# all signal-handler return to this "function"
sigRetFunc:
	mov		$SYSCALL_ACKSIG,%eax
	int		$SYSCALL_IRQ
	# never reached

.ifdef CALLTRACE_PID
.section .data
lookupDepth:
	.long 0
lookupStack:
	.rept 100
		.long 0
	.endr
.endif
