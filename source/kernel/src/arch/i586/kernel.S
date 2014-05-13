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

#include <sys/arch/i586/mem/layout.h>

#define USER_STACK			0xA0000000
#define KSTACK_CURTHREAD	0xFD800FFC

// exports
.global bspEntry
.global apEntry
.global thread_startup
.global proc0PDir
.global kernelStack
.global aplock

// imports
.extern bspstart
.extern apstart
.extern smpstart
.extern unittest_start
.extern uenv_setupThread

.section .bss
// first page directory
.align PAGE_SIZE
proc0PDir:
.rept TMP_STACK_SIZE
	.byte 0
.endr

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
.set MULTIBOOT_HEADER_FLAGS,		MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO
.set MULTIBOOT_CHECKSUM,			-(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

// Multiboot header (needed to boot from GRUB)
.align 4
.long		MULTIBOOT_HEADER_MAGIC
.long		MULTIBOOT_HEADER_FLAGS
.long		MULTIBOOT_CHECKSUM

apEntry:
.code16
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

	// code selector 0x08: base 0x00000000, limit 0xFFFFFFFF, type 0x9A, granularity 0xCF
	.byte 0xFF, 0xFF, 0, 0, 0, 0x9A, 0xCF, 0x00

	// data selector 0x10: base 0x00000000, limit 0xFFFFFFFF, type 0x92, granularity 0xCF
	.byte 0xFF, 0xFF, 0, 0, 0, 0x92, 0xCF, 0x00
setupGDTEntriesEnd:

.code32

// the kernel entry point
bspEntry:
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

	// note that we assume here that the kernel is not larger than 1 page-table (4MiB)!

	// setup paging
	mov		$proc0PDir,%eax
	// virt -> phys
	and		$~KERNEL_AREA,%eax
	// put PDE in slot 0: PDE_EXISTS | PDE_GLOBAL | PDE_LARGE | PDE_WRITABLE | PDE_PRESENT
	movl	$0x00000383,(%eax)
	// put PDE in slot for kernel-area
	add		$(KERNEL_AREA >> (10 + 12 - 2)),%eax
	movl	$0x00000383,(%eax)
	// just to be sure
	and		$~(PAGE_SIZE - 1),%eax
	// set pagedir
	mov		%eax,%cr3
	// enable global pages and large pages
	mov		%cr4,%eax
	or		$0x90,%eax
	mov		%eax,%cr4
	// enable paging
	mov		%cr0,%eax
	or		$0x80000000,%eax
	mov		%eax,%cr0

	// jump to the higher half kernel
	ljmp	$0x08,$higherhalf

.section .text

higherhalf:
	// are we an AP?
	test	%esi,%esi
	jnz		2f

	// for APs: ensure that the initialization is done by one cpu at a time; this way, we can use
	// one temporary stack which simplifies it a bit.
	mov		$1,%ecx
1:
	xor		%eax,%eax
	lock
	cmpxchg %ecx,(aplock)
	jnz		1b

2:
	cli								// disable interrupts during startup
	mov		$kernelStack,%esp		// set up a new stack for our kernel
	mov		%esp,%ebp
	push	%ebp					// push ebp on the stack to ensure that the stack-trace works

	// are we an AP?
	test	%esi,%esi
	jnz		1f
	call	apstart
	// we'll not get here
2:
	jmp		2b

1:
	mov		%ebx,%eax				// pass address of Multiboot-Structure to bspstart
	call	bspstart

	// now use the kernel-stack of the first thread and start smp
	mov		$KSTACK_CURTHREAD,%esp
	sub		$4,%esp
	mov		%esp,%ebp
	push	%ebp
	call	smpstart

	// if we have started the unittest-kernel, we will register the tests here
	mov		%eax,%ebx				// save entry-point
	call	unittest_start
	mov		%ebx,%eax

	// entrypoint is in eax
	mov		$USER_STACK - 4,%ecx	// user stackpointer
	jmp		3f

	// setup env for first task
thread_startup:
	mov		4(%esp),%edx			// load entry-point
	cmp		$KERNEL_AREA,%edx
	jb		2f
	// stay in kernel
	call	*%edx
1:
	jmp		1b

	// setup user-env
2:
	mov		(%esp),%eax
	call	uenv_setupThread		// call uenv_setupThread(arg,entryPoint)
	mov		%eax,%ecx				// user-stackpointer
	mov		8(%esp),%eax			// process entryPoint

	// go to user-mode
3:
	pushl	$0x23					// ss
	pushl	%ecx					// esp
	pushfl							// eflags
	mov		(%esp),%ecx
	or		$1 << 9,%ecx			// enable IF-flag
	// set IOPL=0 (if CPL <= IOPL the user can change eflags..)
	and		$~((1 << 12) | (1 << 13)),%ecx
	mov		%ecx,(%esp)
	pushl	$0x1B					// cs
	push	%eax					// eip (entry-point)
	mov		$0x23,%eax				// set the value for the segment-registers
	mov		%eax,%ds				// reload segments
	mov		%eax,%es
	mov		%eax,%fs
	mov		$0x2B,%eax
	mov		%eax,%gs				// TLS segment
	iret							// jump to task and switch to user-mode
