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

#if defined(__i586__)
#	include <sys/arch/i586/mem/layout.h>
#	include <sys/arch/i586/intrptstackframe.h>
#else
#	include <sys/arch/x86_64/mem/layout.h>
#	include <sys/arch/x86_64/intrptstackframe.h>
#endif
#include <esc/arch.h>

#define KSTACK_CURTHREAD	(KSTACK_AREA + PAGE_SIZE - WORDSIZE)

// exports
.global bspEntry
.global apEntry
.global thread_startup
.global kernelStack
.global aplock
.global higherhalf

// imports
.extern bspstart
.extern apstart
.extern smpstart
.extern unittest_start
.extern uenv_setupThread
.extern paging_setup
.extern gem5boot

.section .initdata
// our temporary kernel stack
.align PAGE_SIZE
.rept TMP_STACK_SIZE
	.byte 0
.endr
kernelStack:

aplock:
	.long	0

// call the section ".init" to force ld to put this section at the beginning of the executable
.section .init

// Multiboot constants
.set MULTIBOOT_PAGE_ALIGN,			1 << 0
.set MULTIBOOT_MEMORY_INFO,			1 << 1
.set MULTIBOOT_HEADER_MAGIC,		0x1BADB002
.set MULTIBOOT_MAGIC,				0x2BADB002
.set MULTIBOOT_HEADER_FLAGS,		MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO
.set MULTIBOOT_CHECKSUM,			-(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

// Multiboot header (needed to boot from GRUB)
.align 4
.long		MULTIBOOT_HEADER_MAGIC
.long		MULTIBOOT_HEADER_FLAGS
.long		MULTIBOOT_CHECKSUM

.code16

apEntry:
	// load the GDT
	lgdtl	%cs:setupGDT - apEntry

	// enable protected mode
	mov		%cr0,%eax
	or		$0x1,%eax
	mov		%eax,%cr0

	// remember that we're an AP
	xor		%esi,%esi

	// far-jump to protected mode
	ljmpl	$0x08,$apProtMode

// our GDT for the setup-process
setupGDT:
	// GDT size
	.word		setupGDTEntriesEnd - setupGDTEntries - 1
	// Pointer to entries
	.long		setupGDTEntries

// the GDT-entries
setupGDTEntries:
	// null gate
	.long 0, 0

#if defined(__i586__)
	// code selector 0x08: base 0x00000000, limit 0xFFFFFFFF, type 0x9A, granularity 0xCF
	.byte 0xFF, 0xFF, 0, 0, 0, 0x9A, 0xCF, 0x00
	// data selector 0x10: base 0x00000000, limit 0xFFFFFFFF, type 0x92, granularity 0xCF
	.byte 0xFF, 0xFF, 0, 0, 0, 0x92, 0xCF, 0x00
#else
	// code selector 0x08: base 0x00000000, limit 0xFFFFFFFF, type 0x9A, granularity 0xA0
	.byte 0xFF, 0xFF, 0, 0, 0, 0x9A, 0xA0, 0x00
	// data selector 0x10: base 0x00000000, limit 0xFFFFFFFF, type 0x92, granularity 0xA0
	.byte 0xFF, 0xFF, 0, 0, 0, 0x92, 0xA0, 0x00
#endif
setupGDTEntriesEnd:

.code32

// the kernel entry point
bspEntry:
#if defined(__x86_64__)
	cmp		$MULTIBOOT_MAGIC,%eax
	// TODO atm, we assume gem5 in this case, which is of course not always true
	jne		gem5boot
#endif

	// we distinguish the BSP and APs by esi
	mov		$1,%esi
	// first setup a GDT
	lgdt	(setupGDT)
	// copy AP-bootstrap code to 0x7000
	mov		$(bspEntry - apEntry),%ecx
	mov		$apEntry,%esi
	mov		$0x7000,%edi
	rep		movsb

apProtMode:
	mov		$0x10,%ax
	mov		%ax,%ds
	mov		%ax,%es
	mov		%ax,%fs
	mov		%ax,%gs
	mov		%ax,%ss

	mov		$1f,%edi
	jmp		paging_setup
1:

	// jump to the higher half kernel
	ljmp	$0x08,$higherhalf

#if defined(__x86_64__)
.code64
#endif

higherhalf:
	// are we an AP?
	test	%REG(si),%REG(si)
	jnz		2f

	// for APs: ensure that the initialization is done by one cpu at a time; this way, we can use
	// one temporary stack which simplifies it a bit.
	mov		$1,%REG(cx)
1:
	xor		%REG(ax),%REG(ax)
	lock
	cmpxchg %REG(cx),(aplock)
	jnz		1b

2:
	cli								// disable interrupts during startup
	mov		$kernelStack,%REG(sp)	// set up a new stack for our kernel
	mov		%REG(sp),%REG(bp)
	push	%REG(bp)				// push bp on the stack to ensure that the stack-trace works

	// are we an AP?
	test	%REG(si),%REG(si)
	jnz		1f
	call	apstart
	// we'll not get here
2:
	jmp		2b

.section .text

1:
	mov		%REG(bx),%ARG_1			// pass address of Multiboot-Structure to bspstart
	call	bspstart

	// now use the kernel-stack of the first thread and start smp
	mov		$KSTACK_CURTHREAD,%REG(sp)
	sub		$WORDSIZE * 2,%REG(sp)
	mov		%REG(sp),%REG(bp)
	push	%REG(bp)
	mov		%REG(sp),%ARG_1
	call	smpstart

	// if we have started the unittest-kernel, we will register the tests here
	mov		%REG(ax),%REG(bx)		// save entry-point
	call	unittest_start
	mov		%REG(bx),%REG(ax)

	// entrypoint is in eax
	// set user stackpointer
	pop		%REG(cx)
	jmp		3f

	// setup env for first task
thread_startup:
	// check if entry-point is in kernel
	mov		WORDSIZE(%REG(sp)),%ARG_2
	cmp		$KERNEL_BEGIN,%ARG_2
	jb		2f
	// stay in kernel
	call	*%ARG_2
1:
	jmp		1b

	// setup user-env
2:
	mov		(%REG(sp)),%ARG_1
	call	uenv_setupThread		// call uenv_setupThread(arg,entryPoint)
	mov		%REG(ax),%REG(cx)		// user-stackpointer
	// process entryPoint
	mov		WORDSIZE * 2(%REG(sp)),%REG(ax)

	// go to user-mode
3:
	mov		%REG(cx),%REG(bp)
	push	$((SEG_UDATA << 3) | 3)	// ss
	push	%REG(cx)				// sp
	push	$0x200					// flags (enable IF-flag)
	push	$((SEG_UCODE2 << 3) | 3)// cs
	push	%REG(ax)				// ip (entry-point)
	mov		$((SEG_UDATA << 3) | 3),%REG(ax)
	mov		%REG(ax),%ds			// reload segments
	mov		%REG(ax),%es
	mov		%REG(ax),%fs
	mov		$((SEG_TLS << 3) | 3),%REG(ax)
	mov		%REG(ax),%gs			// TLS segment
	IRET							// jump to task and switch to user-mode
