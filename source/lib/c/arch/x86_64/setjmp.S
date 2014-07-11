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

.section .text

.global setjmp
.global longjmp

// int setjmp(jmp_buf env);
.type setjmp, @function
setjmp:
	push	%rbp
	mov		%rsp,%rbp

	// save regs (callee-save)
	mov		%rbx,  0(%rdi)
	mov		%rsp,  8(%rdi)
	mov		%r12, 16(%rdi)
	mov		%r13, 24(%rdi)
	mov		%r14, 32(%rdi)
	mov		%r15, 40(%rdi)
	mov		(%rbp),%rcx							// store rbp of the caller stack-frame
	mov		%rcx, 48(%rdi)
	pushf										// load eflags
	pop		56(%rdi)							// store
	mov		8(%rbp),%rcx						// store return-address
	mov		%rcx,64(%rdi)

	mov		$0,%rax								// return 0
	leave
	ret

// int longjmp(jmp_buf env,int val);
.type longjmp, @function
longjmp:
	push	%rbp
	mov		%rsp,%rbp

	// restore registers (callee-save)
	mov		 0(%rdi),%rbx
	mov		 8(%rdi),%rsp
	add		$8,%rsp								// undo 'push rbp'
	mov		16(%rdi),%r12
	mov		24(%rdi),%r13
	mov		32(%rdi),%r14
	mov		40(%rdi),%r15
	mov		48(%rdi),%rbp
	push	56(%rdi)							// get eflags
	popf										// restore
	mov		64(%rdi),%rax						// get return-address
	mov		%rax,(%rsp)							// set return-address

	mov		%rsi,%rax							// return val
	ret											// no leave here because we've already restored the
												// rbp of the caller stack-frame
