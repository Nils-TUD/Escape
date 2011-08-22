/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

.global thread_idle
.global thread_save
.global thread_resume

# process save area offsets
.set STATE_ESP,						0
.set STATE_EDI,						4
.set STATE_ESI,						8
.set STATE_EBP,						12
.set STATE_EFLAGS,					16
.set STATE_EBX,						20

# void thread_idle(void);
thread_idle:
	sti
1:
	hlt
	jmp			1b

# bool thread_save(sThreadRegs *saveArea);
thread_save:
	push	%ebp
	mov		%esp,%ebp

	# save register
	mov		8(%ebp),%eax			# get saveArea
	mov		%ebx,STATE_EBX(%eax)
	mov		%esp,STATE_ESP(%eax)	# store esp
	mov		%edi,STATE_EDI(%eax)
	mov		%esi,STATE_ESI(%eax)
	mov		%ebp,STATE_EBP(%eax)
	pushfl							# load eflags
	popl	STATE_EFLAGS(%eax)		# store

	mov		$0,%eax					# return 0
	leave
	ret

# bool thread_resume(tPageDir pageDir,sThreadRegs *saveArea,klock_t lock);
thread_resume:
	push	%ebp
	mov		%esp,%ebp

	mov		8(%ebp),%edi			# get page-dir
	mov		12(%ebp),%eax			# get saveArea
	mov		16(%ebp),%edx			# get lock

	mov		%edi,%cr3				# set page-dir

	# now restore registers
	mov		STATE_EDI(%eax),%edi
	mov		STATE_ESI(%eax),%esi
	mov		STATE_EBP(%eax),%ebp
	mov		STATE_ESP(%eax),%esp
	mov		STATE_EBX(%eax),%ebx
	pushl	STATE_EFLAGS(%eax)
	popfl							# load eflags
	movl	$0,(%edx)				# unlock now; the old thread can be used

	mov		$1,%eax					# return 1
	leave
	ret
