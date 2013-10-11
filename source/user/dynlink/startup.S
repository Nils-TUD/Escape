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

#include <esc/syscalls.h>

.global sigRetFunc
.global _start
.global lookup_resolveStart
.extern __libc_preinit
.extern __libc_init
.extern lookup_resolve
.extern load_initHeap
.extern load_setupProg
.extern getpid_wrapper

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
	call	load_initHeap
	# call __libc_preinit first
	mov		(%esp),%ebx
	mov		4(%esp),%eax
	mov		8(%esp),%edx
	mov		12(%esp),%ecx
	add		$16,%esp
	call	__libc_preinit
	push	%ecx
	push	%edx
	push	%eax
	# now call __libc_init
	call	__libc_init
	# pass pointers to TLSSize and TLSStart to load_setupProg
	lea		4(%esp),%edx
	lea		8(%esp),%ecx
	mov		%ebx,%eax
	call	load_setupProg
	# load_setupProg returns the entrypoint
	jmp		*%eax

lookup_resolveStart:
#ifndef CALLTRACE_PID
	push	%eax
	push	%edx
	push	%ecx
	call	lookup_resolve
	push	%eax
	mov		4(%esp),%ecx
	mov		8(%esp),%edx
	mov		12(%esp),%eax
	add		$24,%esp								# remove args of lookup_resolve from stack
	jmp		*-24(%esp)								# jump to function
#else
	push	8(%esp)									# push ret-addr for the call
	push	%eax
	push	%edx
	push	%ecx
	call	lookup_resolve
	push	%eax
	call	getpid_wrapper							# get pid
	cmp		$CALLTRACE_PID,%eax						# if not the requested pid skip the whole stuff
	jne		1f
	mov		(lookupDepth),%ecx
	cmp		$100,%ecx								# we have only 100 places...
	jae		1f
	mov		$lookupStack,%edx
	lea		(%edx,%ecx,4),%edx						# load addr to store ret-addr at
	add		$1,%ecx									# inc depth
	mov		%ecx,(lookupDepth)						# store
	mov		28(%esp),%ecx							# get real return-addr
	mov		%ecx,(%edx)								# save
	mov		$lookup_resolveFinish,%ecx				# replace with our own
	mov		%ecx,28(%esp)
1:
	mov		4(%esp),%ecx
	mov		8(%esp),%edx
	mov		12(%esp),%eax
	add		$28,%esp								# remove args of lookup_resolve from stack
	jmp		*-28(%esp)								# jump to function

lookup_resolveFinish:
	.extern lookup_tracePop
	push	%eax
	call	lookup_tracePop
	pop		%eax
	mov		(lookupDepth),%ecx						# load depth
	sub		$1,%ecx									# decrement
	mov		%ecx,(lookupDepth)						# store
	mov		$lookupStack,%edx						# get stack
	lea		(%edx,%ecx,4),%edx						# build addr to our ret-addr
	mov		(%edx),%ecx								# get ret-addr
	jmp		*%ecx									# jump to that addr
#endif

# all signal-handler return to this "function"
sigRetFunc:
	mov		$SYSCALL_ACKSIG,%eax
	int		$ACKSIG_IRQ
	# never reached

#ifdef CALLTRACE_PID
.section .data
lookupDepth:
	.long 0
lookupStack:
	.rept 100
		.long 0
	.endr
#endif
