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

.global thread_save
.global thread_resume

// process save area offsets
.set STATE_RBX,						0
.set STATE_RSP,						8
.set STATE_RBP,						16
.set STATE_R12,						24
.set STATE_R13,						32
.set STATE_R14,						40
.set STATE_R15,						48
.set STATE_RFLAGS,					56

// bool thread_save(ThreadRegs *saveArea);
thread_save:
	push	%rbp
	mov		%rsp,%rbp

	// save register
	mov		%rbx,STATE_RBX(%rdi)
	mov		%rsp,STATE_RSP(%rdi)
	mov		%rbp,STATE_RBP(%rdi)
	mov		%r12,STATE_R12(%rdi)
	mov		%r13,STATE_R13(%rdi)
	mov		%r14,STATE_R14(%rdi)
	mov		%r15,STATE_R15(%rdi)
	pushf							// load eflags
	pop		STATE_RFLAGS(%rdi)		// store

	mov		$0,%rax					// return 0
	leave
	ret

// bool thread_resume(uintptr_t pageDir,ThreadRegs *saveArea,uint *lock);
thread_resume:
	push	%rbp
	mov		%rsp,%rbp

	// exchange page-dir, if necessary
	test	%rdi,%rdi
	jz		1f						// dont set the page-dir if its the same process
	mov		%rdi,%cr3				// set page-dir

	// now restore registers
1:
	mov		STATE_R15(%rsi),%r15
	mov		STATE_R14(%rsi),%r14
	mov		STATE_R13(%rsi),%r13
	mov		STATE_R12(%rsi),%r12
	mov		STATE_RBP(%rsi),%rbp
	mov		STATE_RSP(%rsi),%rsp
	mov		STATE_RBX(%rsi),%rbx
	push	STATE_RFLAGS(%rsi)
	popf							// load eflags

	// unlock now; the old thread can be used
	movl	$0,(%rdx)

	mov		$1,%rax					// return 1
	leave
	ret
