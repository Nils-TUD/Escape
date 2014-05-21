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
.set STATE_ESP,						0
.set STATE_EDI,						4
.set STATE_ESI,						8
.set STATE_EBP,						12
.set STATE_EFLAGS,					16
.set STATE_EBX,						20

// bool thread_save(ThreadRegs *saveArea);
thread_save:
	push	%ebp
	mov		%esp,%ebp

	// save register
	mov		%ebx,STATE_EBX(%eax)
	mov		%esp,STATE_ESP(%eax)	// store esp
	mov		%edi,STATE_EDI(%eax)
	mov		%esi,STATE_ESI(%eax)
	mov		%ebp,STATE_EBP(%eax)
	pushfl							// load eflags
	popl	STATE_EFLAGS(%eax)		// store

	mov		$0,%eax					// return 0
	leave
	ret

// bool thread_resume(uintptr_t pageDir,ThreadRegs *saveArea,uint *lock,bool newProc);
thread_resume:
	push	%ebp
	mov		%esp,%ebp

	// exchange page-dir, if necessary
	mov		8(%ebp),%edi			// get newProc
	test	%edi,%edi
	jz		1f						// dont set the page-dir if its the same process
	mov		%eax,%cr3				// set page-dir

	// now restore registers
1:
	mov		STATE_EDI(%edx),%eax
	mov		STATE_ESI(%edx),%esi
	mov		STATE_EBP(%edx),%ebp
	mov		STATE_ESP(%edx),%esp
	mov		STATE_EBX(%edx),%ebx
	pushl	STATE_EFLAGS(%edx)
	popfl							// load eflags

	// unlock now; the old thread can be used
	movl	$0,(%ecx)

	mov		$1,%eax					// return 1
	leave
	ret
