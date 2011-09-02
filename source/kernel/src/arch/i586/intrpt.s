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

.global waiting
.global waitlock
.global halting
.extern apic_eoi
.extern intrpt_handler

waiting:
	.long	0
waitlock:
	.long	0
halting:
	.long	0

# macro to build a default-isr-handler
.macro BUILD_DEF_ISR no
	.global isr\no
	isr\no:
	# interrupts are already disabled here since its a interrupt-gate, not a trap-gate
	pushl	$0						# error-code (no error here)
	pushl	$\no					# the interrupt-number
	jmp		isrCommon
.endm

# macro to build an error-isr-handler
.macro BUILD_ERR_ISR no
	.global isr\no
	isr\no:
	# interrupts are already disabled here since its a interrupt-gate, not a trap-gate
	# the error-code has already been pushed
	pushl	$\no					# the interrupt-number
	jmp		isrCommon
.endm

# our ISRs
BUILD_DEF_ISR 0
BUILD_DEF_ISR 1
BUILD_DEF_ISR 2
BUILD_DEF_ISR 3
BUILD_DEF_ISR 4
BUILD_DEF_ISR 5
BUILD_DEF_ISR 6
BUILD_DEF_ISR 7
BUILD_ERR_ISR 8
BUILD_DEF_ISR 9
BUILD_ERR_ISR 10
BUILD_ERR_ISR 11
BUILD_ERR_ISR 12
BUILD_ERR_ISR 13
BUILD_ERR_ISR 14
BUILD_DEF_ISR 15
BUILD_DEF_ISR 16
BUILD_ERR_ISR 17
BUILD_DEF_ISR 18
BUILD_DEF_ISR 19
BUILD_DEF_ISR 20
BUILD_DEF_ISR 21
BUILD_DEF_ISR 22
BUILD_DEF_ISR 23
BUILD_DEF_ISR 24
BUILD_DEF_ISR 25
BUILD_DEF_ISR 26
BUILD_DEF_ISR 27
BUILD_DEF_ISR 28
BUILD_DEF_ISR 29
BUILD_DEF_ISR 30
BUILD_DEF_ISR 31
BUILD_DEF_ISR 32
BUILD_DEF_ISR 33
BUILD_DEF_ISR 34
BUILD_DEF_ISR 35
BUILD_DEF_ISR 36
BUILD_DEF_ISR 37
BUILD_DEF_ISR 38
BUILD_DEF_ISR 39
BUILD_DEF_ISR 40
BUILD_DEF_ISR 41
BUILD_DEF_ISR 42
BUILD_DEF_ISR 43
BUILD_DEF_ISR 44
BUILD_DEF_ISR 45
BUILD_DEF_ISR 46
BUILD_DEF_ISR 47
BUILD_DEF_ISR 48
BUILD_DEF_ISR 49

# IPI: flush TLB
.global isr50
	isr50:
	pusha
	mov		%cr3,%eax
	mov		%eax,%cr3
	call	apic_eoi
	popa
	iret

# IPI: wait
.global isr51
	isr51:
	pusha
	# tell the CPU that requested the wait that we're waiting
	lock
	add		$1,(waiting)
	# now wait until its unlocked
1:
	mov		(waitlock),%eax
	test	%eax,%eax
	jz		2f
	pause
	jmp		1b
2:	call	apic_eoi
	popa
	iret

# IPI: halt
.global isr52
	isr52:
	# tell the CPU that requested the wait that we're halting
	lock
	add		$1,(halting)
	# now hlt here with interrupts disabled
1:
	hlt
	# not reached
	jmp		1b

# our null-handler for all other interrupts
.global isrNull
	isrNull:
	# interrupts are already disabled here since its a interrupt-gate, not a trap-gate
	pushl	$0						# error-code (no error here)
	pushl	$53						# the interrupt-number
	jmp		isrCommon

# the ISR for all interrupts
isrCommon:
	# save registers
	pusha
	push	%gs
	push	%fs
	push	%ds
	push	%es

	# set segment-registers for kernel
	# TODO just necessary for VM86-tasks because ds,es,fs and gs is set to 0 in this case
	mov		$0x23,%eax
	mov		%eax,%ds
	mov		%eax,%es
	mov		%eax,%fs
	mov		%eax,%gs

	# call c-routine
	push	%esp					# stack-frame
	mov		%esp,%eax
	push	%eax					# pointer to stack-frame
	call	intrpt_handler

	# remove arguments from stack
	add		$8,%esp

	# restore registers
	pop		%es
	pop		%ds
	pop		%fs
	pop		%gs
	popa

	# remove error-code and interrupt-number from stack
	add		$8,%esp

	# return
	iret
