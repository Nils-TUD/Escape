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

# macros for the different syscall-types, depending on the argument-number

.set	ACKSIG_IRQ,		49

.macro SYSC_DEBUG name
.global \name
.type \name, @function
\name:
	int		$0x30
	ret
.endm

.macro SYSC_0ARGS name syscno
.global \name
.type \name, @function
\name:
	push	%ebp
	mov		%esp,%ebp
	push	%edi
	mov		$\syscno,%eax					# set syscall-number
	DO_SYSENTER
	STORE_ERRNO
	pop		%edi
	leave
	ret
.endm

.macro SYSC_1ARGS name syscno
.global \name
.type \name, @function
\name:
	push	%ebp
	mov		%esp,%ebp
	push	%edi
	push	%esi
	mov		%eax,%esi						# set arg1
	mov		$\syscno,%eax					# set syscall-number
	DO_SYSENTER
	STORE_ERRNO
	pop		%esi
	pop		%edi
	leave
	ret
.endm

.macro SYSC_2ARGS name syscno
.global \name
.type \name, @function
\name:
	push	%ebp
	mov		%esp,%ebp
	push	%edi
	push	%esi
	mov		%eax,%esi						# set arg1
	mov		%edx,%edi						# set arg2
	mov		$\syscno,%eax					# set syscall-number
	DO_SYSENTER
	STORE_ERRNO
	pop		%esi
	pop		%edi
	leave
	ret
.endm

.macro SYSC_3ARGS name syscno
.global \name
.type \name, @function
\name:
	push	%ebp
	mov		%esp,%ebp
	push	%edi
	push	%esi
	push	%ebx
	mov		%eax,%esi						# set arg1
	mov		%edx,%edi						# set arg2
	mov		%ecx,%ebx						# set arg3
	mov		$\syscno,%eax					# set syscall-number
	DO_SYSENTER
	STORE_ERRNO
	pop		%ebx
	pop		%esi
	pop		%edi
	leave
	ret
.endm

.macro SYSC_4ARGS name syscno
.global \name
.type \name, @function
\name:
	push	%ebp
	mov		%esp,%ebp
	push	%edi
	push	%esi
	push	%ebx
	mov		%eax,%esi						# set arg1
	mov		%edx,%edi						# set arg2
	mov		%ecx,%ebx						# set arg3
	pushl	8(%ebp)							# push arg4
	mov		$\syscno,%eax					# set syscall-number
	DO_SYSENTER
	add		$4,%esp							# remove arg4
	STORE_ERRNO
	pop		%ebx
	pop		%esi
	pop		%edi
	leave
	ret
.endm

.macro SYSC_5ARGS name syscno
.global \name
.type \name, @function
\name:
	push	%ebp
	mov		%esp,%ebp
	push	%edi
	push	%esi
	push	%ebx
	mov		%eax,%esi						# set arg1
	mov		%edx,%edi						# set arg2
	mov		%ecx,%ebx						# set arg3
	pushl	12(%ebp)						# push arg5
	pushl	8(%ebp)							# push arg4
	mov		$\syscno,%eax					# set syscall-number
	DO_SYSENTER
	add		$8,%esp							# remove args
	STORE_ERRNO
	pop		%ebx
	pop		%esi
	pop		%edi
	leave
	ret
.endm

.macro SYSC_6ARGS name syscno
.global \name
.type \name, @function
\name:
	push	%ebp
	mov		%esp,%ebp
	push	%edi
	push	%esi
	push	%ebx
	mov		%eax,%esi						# set arg1
	mov		%edx,%edi						# set arg2
	mov		%ecx,%ebx						# set arg3
	pushl	16(%ebp)						# push arg6
	pushl	12(%ebp)						# push arg5
	pushl	8(%ebp)							# push arg4
	mov		$\syscno,%eax					# set syscall-number
	DO_SYSENTER
	add		$12,%esp						# remove args
	STORE_ERRNO
	pop		%ebx
	pop		%esi
	pop		%edi
	leave
	ret
.endm

.macro SYSC_7ARGS name syscno
.global \name
.type \name, @function
\name:
	push	%ebp
	mov		%esp,%ebp
	push	%edi
	push	%esi
	push	%ebx
	mov		%eax,%esi						# set arg1
	mov		%edx,%edi						# set arg2
	mov		%ecx,%ebx						# set arg3
	pushl	20(%ebp)						# push arg7
	pushl	16(%ebp)						# push arg6
	pushl	12(%ebp)						# push arg5
	pushl	8(%ebp)							# push arg4
	mov		$\syscno,%eax					# set syscall-number
	DO_SYSENTER
	add		$16,%esp						# remove args
	STORE_ERRNO
	pop		%ebx
	pop		%esi
	pop		%edi
	leave
	ret
.endm

# calculates the return address for sysenter and the stackpointer and performs sysenter.
# this has to be done differently when using it in a shared library because we don't know the
# absolute address
.macro DO_SYSENTER
#ifdef SHAREDLIB
	call	1f
1:
	pop		%edx
	add		$(1f - 1b),%edx
#else
	mov		$1f,%edx
#endif
	mov		%esp,%ecx
	sysenter
1:
.endm

# stores the received error-code (in edi) to the global variable errno
.macro STORE_ERRNO
	test	%edi,%edi
	jz		1f								# no-error?
	# use GOT for shared-libraries
	#ifdef SHAREDLIB
	# first get address of GOT
	call	__i686.get_pc_thunk.bx
	addl	$_GLOBAL_OFFSET_TABLE_,%eax
	# now access symbol and store errno
	mov		errno@GOT(%eax),%eax
	mov		%edi,(%eax)
	negl	(%eax)
	#else
	# otherwise access errno directly
	mov		%edi,(errno)					# store error-code
	negl	(errno)
	#endif
	mov		%edi,%eax						# return error-code
1:
.endm
