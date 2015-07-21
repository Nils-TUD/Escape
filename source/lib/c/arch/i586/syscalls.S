/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <sys/asm.h>
#include <sys/syscalls.h>

.section .text

// calculates the return address for sysenter and the stackpointer and performs sysenter.
// this has to be done differently when using it in a shared library because we don't know the
// absolute address
.macro DO_SYSENTER
#if defined(SHAREDLIB)
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

// stores the received error-code (in edi) to the global variable errno
.macro STORE_ERRNO
	test	%edi,%edi
	jz		1f								// no-error?
#if defined(SHAREDLIB)
	call	errno_location@plt
#else
	call	errno_location
#endif
	mov		%edi,(%eax)
	negl	(%eax)
	mov		%edi,%eax						// return error-code
1:
.endm

// needed for shared libraries to get the address of the GOT
#if defined(SHAREDLIB)
// put in the linkonce-section to tell the linker that he should put this function in the
// executable / shared library just once
.section .gnu.linkonce.t.__i686.get_pc_thunk.bx,"ax",@progbits
.hidden  __i686.get_pc_thunk.bx
BEGIN_FUNC(__i686.get_pc_thunk.bx)
	movl (%esp),%eax
	ret
END_FUNC(__i686.get_pc_thunk.bx)
.previous
#endif

BEGIN_FUNC(syscall7)
	push	%ebp
	mov		%esp,%ebp
	push	%edi
	push	%esi
	mov		%edx,%esi						// set arg1
	mov		%ecx,%edi						// set arg2
	pushl	24(%ebp)						// push arg7
	pushl	20(%ebp)						// push arg6
	pushl	16(%ebp)						// push arg5
	pushl	12(%ebp)						// push arg4
	pushl	8(%ebp)							// push arg3
	DO_SYSENTER
	add		$20,%esp						// remove args
	STORE_ERRNO
	pop		%esi
	pop		%edi
	leave
	ret
END_FUNC(syscall7)
