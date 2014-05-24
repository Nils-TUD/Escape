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

#include <sys/intrptstackframe.h>
#include <esc/arch.h>

.global kstackPtr
.global waiting
.global waitlock
.global halting
.global flushed
.extern lapic_eoi
.extern intrpt_handler
.extern syscall_handler

.align 8
kstackPtr:
	.quad	0
waiting:
	.long	0
waitlock:
	.long	0
halting:
	.long	0
flushed:
	.long	0

// macro to build a default-isr-handler
.macro BUILD_DEF_ISR no
	.global isr\no
	isr\no:
	// interrupts are already disabled here since its a interrupt-gate, not a trap-gate
	push	$0						// error-code (no error here)
	push	$\no					// the interrupt-number
	jmp		isrCommon
.endm

// macro to build an error-isr-handler
.macro BUILD_ERR_ISR no
	.global isr\no
	isr\no:
	// interrupts are already disabled here since its a interrupt-gate, not a trap-gate
	// the error-code has already been pushed
	push	$\no					// the interrupt-number
	jmp		isrCommon
.endm

// our ISRs
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
BUILD_DEF_ISR 50
BUILD_DEF_ISR 51
BUILD_DEF_ISR 56

// IPI: flush TLB
.global isr52
isr52:
	SAVE_REGS
	// flush TLB
	mov		%cr3,%REG(ax)
	mov		%REG(ax),%cr3
	call	lapic_eoi
	RESTORE_REGS
	IRET

// IPI: wait
.global isr53
isr53:
	SAVE_REGS
	// tell the CPU that requested the wait that we're waiting
	lock
	add		$1,(waiting)
	// now wait until its unlocked
1:
	mov		(waitlock),%REG(ax)
	test	%REG(ax),%REG(ax)
	jz		2f
	pause
	jmp		1b
2:	call	lapic_eoi
	// tell the CPU that we're done
	lock
	sub		$1,(waiting)
	RESTORE_REGS
	IRET

// IPI: halt
.global isr54
isr54:
	// tell the CPU that requested the halt that we're halting
	lock
	add		$1,(halting)
	// now hlt here with interrupts disabled
1:
	hlt
	// not reached
	jmp		1b

// IPI: flush TLB ack
.global isr55
isr55:
	SAVE_REGS
	// flush TLB
	mov		%cr3,%REG(ax)
	mov		%REG(ax),%cr3
	// tell the CPU that requested the flush that we've flushed our TLB
	lock
	add		$1,(flushed)
	// continue
	call	lapic_eoi
	RESTORE_REGS
	IRET

// our null-handler for all other interrupts
.global isrNull
isrNull:
	// interrupts are already disabled here since its a interrupt-gate, not a trap-gate
	push	$0						// error-code (no error here)
	push	$57						// the interrupt-number
	jmp		isrCommon

// syscall
.global syscall_entry
syscall_entry:
	SYS_ENTER
	push	$0						// store intrptNo = 0 so that we can distinguish the stack-frames
	SAVE_REGS

	// call c-routine
	mov		%REG(sp),%ARG_1			// pointer to stack-frame
	call	syscall_handler

	RESTORE_REGS
	SYS_LEAVE

// the ISR for all interrupts
isrCommon:
	SAVE_REGS

#if defined(__i586__)
	// set segment-registers for kernel
	// TODO just necessary for VM86-tasks because ds,es,fs and gs is set to 0 in this case
	mov		$((SEG_UDATA << 3) | 3),%eax
	mov		%eax,%ds
	mov		%eax,%es
	mov		%eax,%fs
	mov		%eax,%gs
#endif

	// call c-routine
	mov		%REG(sp),%ARG_1			// pointer to stack-frame
	call	intrpt_handler

	RESTORE_REGS

	// remove error-code and interrupt-number from stack and return
	add		$(WORDSIZE * 2),%REG(sp)
	IRET
