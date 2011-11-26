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

# exports
.global thread_startup
.global kernelStack
.global higherhalf
.global apProtMode
.global aplock

# imports
.extern bspstart
.extern apstart
.extern smpstart
.extern entryPoint
.extern uenv_setupThread

# general constants
.set PAGE_SIZE,						4096
.set TMP_STACK_SIZE,				PAGE_SIZE
.set USER_STACK,					0xA0000000
.set KERNEL_AREA,					0xC0000000
.set KSTACK_CURTHREAD,				0xFD800FFC

# Multiboot constants
.set MULTIBOOT_PAGE_ALIGN,			1 << 0
.set MULTIBOOT_MEMORY_INFO,			1 << 1
.set MULTIBOOT_HEADER_MAGIC,		0x1BADB002
.set MULTIBOOT_HEADER_FLAGS,		MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO
.set MULTIBOOT_CHECKSUM,			-(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

.section .text

# Multiboot header (needed to boot from GRUB)
.align 4
.long		MULTIBOOT_HEADER_MAGIC
.long		MULTIBOOT_HEADER_FLAGS
.long		MULTIBOOT_CHECKSUM

higherhalf:
	# from now the CPU will translate automatically every address
	# by adding the base 0x40000000

	cli								# disable interrupts during startup
	mov		$kernelStack,%esp		# set up a new stack for our kernel
	mov		%esp,%ebp
	push	%ebp					# push ebp on the stack to ensure that the stack-trace works

	push	%eax					# push Multiboot Magicnumber onto the stack
	push	%ebx					# push address of Multiboot-Structure
	call	bspstart				# jump to our C kernel (returns entry-point)
	add		$8,%esp					# remove args from stack

	# now use the kernel-stack of the first thread and start smp
	mov		$KSTACK_CURTHREAD,%esp
	sub		$4,%esp
	mov		%esp,%ebp
	push	%ebp
	call	smpstart
	# entrypoint is in eax
	mov		$USER_STACK - 4,%ecx	# user stackpointer
	jmp		3f

	# setup env for first task
thread_startup:
	mov		4(%esp),%eax			# load entry-point
	cmp		$KERNEL_AREA,%eax
	jb		2f
	# stay in kernel
	call	*%eax
1:
	jmp		1b

	# setup user-env
2:
	call	uenv_setupThread		# call uenv_setupThread(arg,entryPoint)
	mov		%eax,%ecx				# user-stackpointer
	mov		8(%esp),%eax			# entryPoint

	# go to user-mode
3:
	pushl	$0x23					# ss
	pushl	%ecx					# esp
	pushfl							# eflags
	mov		(%esp),%ecx
	or		$1 << 9,%ecx			# enable IF-flag
	# set IOPL=0 (if CPL <= IOPL the user can change eflags..)
	and		$~((1 << 12) | (1 << 13)),%ecx
	mov		%ecx,(%esp)
	pushl	$0x1B					# cs
	push	%eax					# eip (entry-point)
	mov		$0x23,%eax				# set the value for the segment-registers
	mov		%eax,%ds				# reload segments
	mov		%eax,%es
	mov		%eax,%fs
	mov		$0x2B,%eax
	mov		%eax,%gs				# TLS segment
	iret							# jump to task and switch to user-mode

aplock:
	.long	0

apProtMode:
	# ensure that the initialization is done by one cpu at a time; this way, we can use one temporary
	# stack which simplifies it a bit.
	mov		$1,%ecx
1:
	xor		%eax,%eax
	lock
	cmpxchg %ecx,(aplock)
	jnz		1b

	# setup stack
	mov		$kernelStack,%esp		# set up a new stack for our kernel
	mov		%esp,%ebp
	push	%ebp					# push ebp on the stack to ensure that the stack-trace works

	call	apstart

	# wait here
1:
	jmp		1b

.section .bss

# our temporary kernel stack
.align PAGE_SIZE
.rept TMP_STACK_SIZE
	.byte 0
.endr
kernelStack:
